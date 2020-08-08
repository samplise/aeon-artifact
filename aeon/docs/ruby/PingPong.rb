#!/usr/bin/ruby
#
# NOTE: this script requires the bindata ruby gem.
#
# Sample ruby script for interacting with the mace PingPong service.  Note --
# the ruby script binds specifically to 127.0.0.1 on a fixed port.  The fixed
# IP was because on my machine, creating a socket letting it bind idly would
# bind to an IPV6 address, and Mace currently only supports IPV4.  Binding to a
# specific port was just so I would not have to change my script as I tested
# it.
#
# A few things are simplified in this code below.  For one, registration_uid_t
# values are ignored.  In general, they are used to identify which user of a
# service such as the transport the message is being delivered to.  Since in
# this example, there is only one higher-level service, it is not a concern
# here.  Another is in ignoring the proxy address.  A version that can talk to
# Mace services through a NAT would handle proxy addresses if set.  Moreover,
# this service responds to messages, so just utilizes the MaceAddr it receives
# as an address for itself, rather than constructing one based on its
# self-detected IP Address and port.
#
# Port offsets are furthermore ignored, as this example has only one transport.  
#
# Simple usage:
#
# (Run this script using ruby in one shell)
# $ ruby PingPong.rb
#
# (Run unit_app with the Mace PingPong service, specifying the remote service
# to be ruby script)
# $ ./unit_app -run_time 30 -service PingPong -MACE_ADDRESS_ALLOW_LOOPBACK 1 -MACE_LOCAL_ADDRESS localhost:5700 -MACE_LOG_AUTO_ALL 1 -ServiceConfig.PingPong.remote IPV4/localhost:10203
#
# This script creates a thread for each client, but does properly handle
# multiple clients simultaneously.

require "rubygems"
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

class TransportMessage < BinData::Record
  endian :big

  mace_addr_bin :src
  mace_addr_bin :dest
  uint32        :registration_uid
  bit8          :flags
  uint32        :len, :value => lambda { data.length }
  string        :data, :read_length => :len
end

class Pong < BinData::Record
  endian :big

  uint8  :message_type, :value => 1
  uint32 :counter
  uint64 :timestamp
end

require "socket"

myserver = TCPServer.new('127.0.0.1',10203)
sockaddr = myserver.addr

puts "Server running on #{sockaddr.join(':')}"

while true
  Thread.start(myserver.accept) do |sock|
    puts "#{sock} connected at #{Time.now}"

    message = TransportMessage.read(sock);
      
    remote_ip = IPAddr.new(message.src.local.addr, Socket::AF_INET)
    proxy_ip = IPAddr.new(message.src.proxy.addr, Socket::AF_INET)
    puts "Got connection from #{remote_ip}:#{message.src.local.port}/#{proxy_ip}:#{message.src.proxy.port}, size #{message.len}, regid #{message.registration_uid}"

    #assumes proxy not used.
    sock2 = TCPsocket.open("#{remote_ip}", "#{message.src.local.port}");

    begin

      while true
        msg = Pong.read(message.data)
        puts "Got Pong message! (timestamp: #{msg.timestamp} counter: #{msg.counter})"

        response = Pong.new
        response.timestamp = msg.timestamp
        response.counter = msg.counter+1
        puts "Response message message_type: #{response.message_type} counter: #{response.counter} timestamp: #{response.timestamp}"

        response_msg = TransportMessage.new
        response_msg.src = message.dest
        response_msg.dest = message.src
        response_msg.registration_uid = message.registration_uid
        response_msg.flags = message.flags

        response_msg.data = response.to_binary_s
        
        response_msg.write(sock2)

        message = TransportMessage.read(sock);
      end

    rescue Exception => e:
      puts "Error handling PingPong client: #{e}"
    ensure
      sock2.close
      sock.close
      $stdout.flush
    end
  end
end
