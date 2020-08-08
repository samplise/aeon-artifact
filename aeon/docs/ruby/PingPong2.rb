#!/usr/bin/env ruby

require 'rubygems'
require 'socket'

require "bindata"
require "ipaddr"

class SockAddrBin < BinData::Record
  endian :big

  uint32 :addr 
  uint16 :port
end

class MaceAddrBin < BinData::Record
  endian :big

  sock_addr_bin :local
  sock_addr_bin :proxy
end

class TransportHeader < BinData::Record
  endian :big

  mace_addr_bin :src
  mace_addr_bin :dest
  uint32        :registration_uid
  bit8          :flags
  uint32        :len
end

class TransportMessage < BinData::Record
  endian :big

  mace_addr_bin :src
  mace_addr_bin :dest
  uint32        :registration_uid
  bit8          :flags
  uint32        :len, :value => lambda { data.length }
  string        :data, :read_length => :len
end


require 'eventmachine'

def local_ip(target)
  # turn off reverse DNS resolution
  bdns, Socket.do_not_reverse_lookup = Socket.do_not_reverse_lookup, true

  UDPSocket.open do |s|
    s.connect target, 8000
    s.addr.last
  end
ensure
  # restore DNS resolution
  Socket.do_not_reverse_lookup = bdns
end 

class Transport
  @@base_port = 10300
  @@current_port = @@base_port
  @@local_addr = MaceAddrBin.new

  def self.setup_address
    ip = local_ip("www.macesystems.org")
    @@local_addr.local.addr = IPAddr.new(ip).to_i
    @@local_addr.local.port = @@base_port
    @@local_addr.proxy.addr = IPAddr.new("255.255.255.255").to_i
    @@local_addr.proxy.port = 0

    puts "Mace Address #{ip}:#{@@base_port}/255.255.255.255:0"
  end

  def deliver_message src, dest, data, registration_uid
    puts "NO ACTION: Message received of size #{data.size} from #{src} for #{dest} on rid #{registration_uid}"
  end
end

class IncomingTransportConnection < EventMachine::Connection
  MessageTooBig = 10000000

  # Called on socket connection
  def initialize *args
    super 

    @my_transport = args[0]
    @port, @ip = Socket.unpack_sockaddr_in(get_peername)
    puts "New socket connected from #{@ip}:#{@port} with port offset #{@my_transport.my_port_offset}"
    @data_received = ""
    @header = TransportHeader.new
    @parsing_header = true
    @bytes_needed = @header.num_bytes
  end

  # Called on incoming data
  def receive_data data
    @data_received << data

    while @data_received.size >= @bytes_needed
      chunk = @data_received.slice!(0,@bytes_needed)
      if @parsing_header 
        @header.read(chunk)
        if (@header.len > MessageTooBig)
          close_connection
        end
        @bytes_needed = @header.len
        @parsing_header = false

      else
        #Decode on debugging only...
        #remote_ip = IPAddr.new(@header.src.local.addr, Socket::AF_INET)
        #proxy_ip = IPAddr.new(@header.src.proxy.addr, Socket::AF_INET)
        #puts "Message of size #{@header.len} received from #{remote_ip}:#{@header.src.local.port}/#{proxy_ip}:#{@header.src.proxy.port} from socket #{@ip}:#{@port}"

        # Deliver Message to recipient
        @my_transport.deliver_message(@header.src, @header.dest, chunk, @header.registration_uid)

        @bytes_needed = @header.num_bytes
        @parsing_header = true
      end
    end
  end

  # Called on socket termination
  def unbind
    puts "Socket termination from #{@ip}:#{@port}"
    $stdout.flush
  end
end

class OutgoingTransportConnection < EventMachine::Connection
  # Called on socket connection
  def initialize *args
    super 

    @my_transport = args[0]
    @my_dest = args[1]

    #@port, @ip = Socket.unpack_sockaddr_in(get_peername)
    #puts "New socket connected to #{@ip}:#{@port} with port offset #{@my_transport.my_port_offset} for dest #{@my_dest}"

    @outgoing_msg = TransportMessage.new
    @outgoing_msg.src = args[2]
    @outgoing_msg.dest = @my_dest
    @outgoing_msg.flags = 0

    @my_transport.add_outgoing_socket(@my_dest, self)

  rescue Exception => e
    puts "Huh: #{e.to_s} trace #{e.backtrace}"
  end

  # Called on incoming data
  def receive_data data
    raise "ERROR: data received on outgoing connection!"
  end

  def send_message data, registration_uid
    $stdout.flush
    @outgoing_msg.registration_uid = registration_uid
    @outgoing_msg.data = data

    send_data(@outgoing_msg.to_binary_s);
  end

  # Called on socket termination
  def unbind
    puts "Socket termination of outgoing connection to dest #{@my_dest}"
    @my_transport.remove_outgoing_socket(@my_dest)
    $stdout.flush
  end
end

class TcpTransport < Transport
  attr_reader :my_port, :my_port_offset

  def initialize
    @my_port = @@current_port
    @my_port_offset = @my_port - @@base_port
    @@current_port += 1

    @handlers = []
    @outgoing_connections = Hash.new
    @queued_messages = Hash.new

    EventMachine::start_server "0.0.0.0", @my_port, IncomingTransportConnection, self
    puts "Listening on port #{@my_port} with port offset #{@my_port_offset}"
  end

  def register_uid rid, handler
    @handlers[rid] = handler
    puts "Registered handler with registration UID #{rid} at transport with port offset #{@my_port_offset}"
  end

  def deliver_message src, dest, data, registration_uid
    h = @handlers[registration_uid]
    if (h != nil) 
      h.deliver_message(src, dest, data, registration_uid)
    else
      puts "No handler registered with UID #{registration_uid} at transport with port offset #{@my_port_offset}"
      super src, dest, data, registration_uid
    end
  end

  def send_message dest, data, registration_uid
    outgoing = @outgoing_connections[dest]
    unless (outgoing == nil)
      outgoing.send_message(data, registration_uid)
    else
      unless (@queued_messages.has_key?(dest))
        @queued_messages[dest] = [data, registration_uid]
        destaddr = IPAddr.new dest.local.addr, Socket::AF_INET
        EventMachine::connect(destaddr.to_s, dest.local.port, OutgoingTransportConnection, self, dest, @@local_addr)
      else 
        @queued_messages[dest].push(data,registration_uid)
      end
    end
  end

  def add_outgoing_socket dest, conn
    raise "Socket already connected to destination #{dest}" if (@outgoing_connections.has_key?(dest))
    raise "No queued data to #{dest}" unless (@queued_messages.has_key?(dest))

    @outgoing_connections[dest] = conn
    while (@queued_messages[dest].size > 0)
      conn.send_message(@queued_messages[dest].shift, @queued_messages[dest].shift)
    end
    @queued_messages.delete(dest)
  end

  def remove_outgoing_socket dest
    raise "Socket not stored to #{dest}" unless (@outgoing_connections.has_key?(dest))
    raise "Why is there queued data to #{dest} when socket closes" if (@queued_messages.has_key?(dest))
    @outgoing_connections.delete(dest)
  end
end

class RegistrationUidFactory
  @@current_rid = 0

  def self.get_id
    rid = @@current_rid
    @@current_rid += 1
    puts "Returning registration UID #{rid}"
    return rid
  end
end

class PingPong
  class Pong < BinData::Record
    endian :big

    uint8  :message_type, :value => 1
    uint32 :counter
    uint64 :timestamp
  end

  def initialize
    @my_transport = TcpTransport.new
    @my_transport_rid = RegistrationUidFactory.get_id
    @my_transport.register_uid @my_transport_rid, self

    @response_message = Pong.new
  end

  def deliver_message src, dest, data, registration_uid
    raise "ERROR registration uid #{registration_uid} != #{my_transport_rid}" if (registration_uid != @my_transport_rid)

    msg = Pong.read(data)
    puts "Got Pong message! (timestamp: #{msg.timestamp} counter: #{msg.counter})"

    @response_message.timestamp = msg.timestamp
    @response_message.counter = msg.counter+1
    puts "Response message message_type: #{@response_message.message_type} counter: #{@response_message.counter} timestamp: #{@response_message.timestamp}"
    
    @my_transport.send_message(src, @response_message.to_binary_s, registration_uid)
  end
end


EventMachine::run {
  Transport.setup_address
  PingPong.new
}
