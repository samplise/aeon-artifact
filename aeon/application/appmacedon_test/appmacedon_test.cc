/* 
 * appmacedon.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, James W. Anderson, Adolfo Rodriguez
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of the contributors, nor their associated universities 
 *      or organizations may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ----END-OF-LEGAL-STUFF---- */

#include "lib/mace_constants.h"

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <sys/wait.h>
#include <string>

#include "lib/NumberGen.h"
#include "lib/mace.h"
#include "lib/MaceTypes.h"
#include "lib/ThreadCreate.h"
#include "lib/SysUtil.h"
#include "lib/LoadMonitor.h"
#include "lib/Accumulator.h"
#include "lib/mace-macros.h"

#include "services/interfaces/MulticastServiceClass.h"
#include "services/interfaces/GroupServiceClass.h"
#include "services/interfaces/RouteServiceClass.h"
#include "load_protocols.h"
#include "lib/ServiceConfig.h"

#include "Util.h"

const MaceKey GROUPID=MaceKey(sha160, 0xcaca1234);   // some arbitrary group id

// This app has two flavors:  a multicast/collect app and a P2P driver.  
// To switch (un)comment the following line:
// #define APP_P2P

// Defining the below causes the app to stream at the specified streaming_rate.
// Otherwise, it will not send any data.
int app_stream = 1;

const int GROUP_MULTICAST = 0;
const int RANDOM_HASH_ROUTE = 1;
const int UNICAST_DEST_ROUTE = 2;

int stream_type = GROUP_MULTICAST;

#define APP_OVERLAY

#define APP_UNICAST 0

RouteServiceClass *globalRoute;
MulticastServiceClass *globalMulticast;

std::deque<GroupServiceClass*> groupServicesToJoin;

using std::string;

// Globals used only by this test program
/* macedon_Agent *globalmacedon; */
double packet_spacing;  // spacing between pkt sends
int streaming_rate; // in kbps
int gotten=0;   // number of pkts received
double avg_lat=0.0;  // the average latency of received pkts
char *message;   // the message we are sending
string* payload;
bool receiver_started; //marks whether the receiver has been started.  Used to drop early packets.

MaceKey local_ip; //This is always the IP address
MaceKey route_local_addr; //This is the return of globalRoute->localAddress
MaceKey multicast_local_addr; //This is the return of globalRoute->localAddress

void start_sender(int, int, int);
void start_receiver(int, int, int);

class MyHandler : public ReceiveDataHandler {

  public:

  MyHandler() : uid(NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal())
  {}

  const int uid;

  void deliver(const MaceKey& source, const MaceKey& dest, const std::string& msg, registration_uid_t serviceUid)
  {  // called by macedon when data is delivered
    static uint data_packet_size = (uint)params::get<int>("data_packet_size");
    static Accumulator* recvCounter = Accumulator::Instance(Accumulator::APPLICATION_RECV);
    if (!receiver_started) {
      return;
    }
    if(msg.size() != data_packet_size) {
      printf("error: incorrect data size %zi (should be %u)\n",msg.size(),data_packet_size);
    }
    if (msg.size() > sizeof(double)) {
      double delay = TimeUtil::timed() - *((double *)msg.data());
      if (delay > 1000.0) {
        printf("exception: weird time sent detected in app: %f %f\n", TimeUtil::timed(), *((double *)msg.data()));
      }
      else {
        if (avg_lat > 0.0)
          avg_lat = (avg_lat * gotten + delay)/(gotten + 1);
        else 
          avg_lat = delay;
      }
    }
    gotten++;
    recvCounter->accumulate(data_packet_size<<3);
    return;
  }

};

MyHandler* appHandler;

std::list<ServiceClass*> thingsToExit;

static const int SOURCE_PORT = 21000;

std::string appidstring;

int main(int argc, char **argv)
{
  params::addRequired("run_length", "The experiment run time in seconds");
  params::addRequired("data_packet_size", "The size in bytes of a data packet for streaming");
  params::addRequired("streaming_time", "The time in seconds to wait before streaming");
  params::addRequired("streaming_rate", "The rate in Kbps to try streaming at");
  params::addRequired("num_clients", "The number of clients for the test");
  //   params::addRequired("source", "One source node");
  //   if (APP_UNICAST) {
  //     params::addRequired("dest", "destination node");
  //   }
  params::loadparams(argc, argv);
  params::print(stdout);
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  params::set("MACE_AUTO_BOOTSTRAP_PEERS", std::string("localhost:")+boost::lexical_cast<std::string>(SOURCE_PORT));

  fflush(NULL);

  load_protocols();

  int num_clients = params::get<int>("num_clients");
  int MACE_PORT = SOURCE_PORT;
  int MACE_PORT_TEMP = SOURCE_PORT;

  for (int i = 0; i < num_clients; i++) {
    MACE_PORT_TEMP += 10;
    if (fork() == 0) {
      MACE_PORT = MACE_PORT_TEMP;
      std::string fname = std::string("out-")+boost::lexical_cast<std::string>(MACE_PORT)+std::string(".log");
      ASSERT(freopen(fname.c_str(), "w", stdout) != NULL);
      break;
    }
  }

  Log::disableDefaultWarning();
  Log::configure();

  {
    params::set("MACE_LOCAL_ADDRESS", std::string("localhost:")+boost::lexical_cast<std::string>(MACE_PORT));

    MaceKey source(std::string("IPV4/localhost:")+boost::lexical_cast<std::string>(SOURCE_PORT));

    appHandler = new MyHandler();

    logThread(0, __PRETTY_FUNCTION__);
    
    LoadMonitor::runLoadMonitor();
    Accumulator::startLogging();

    ADD_SELECTORS("main");

    int run_length = params::get<int>("run_length");
    int join_time = 0;
    if(params::containsKey("join_time")) { join_time = params::get<int>("join_time"); }
    int streaming_time = 0;
    if(params::containsKey("streaming_time")) { streaming_time = params::get<int>("streaming_time"); }

    if(params::containsKey("app_stream")) { app_stream = params::get<int>("app_stream"); }
    if(params::containsKey("group_multicast")) { 
      std::string s = params::get<std::string>("group_multicast");
      if(s == "GROUP_MULTICAST") {
        stream_type = GROUP_MULTICAST;
      } else if(s == "RANDOM_HASH_ROUTE") {
        stream_type = RANDOM_HASH_ROUTE;
      } else if(s == "UNICAST_DEST_ROUTE") {
        stream_type = UNICAST_DEST_ROUTE;
      } else {
        ASSERT(0);
      }
    }

    maceLog("Source is %s\n",source.toString().c_str());

    // Allocate a message and fill it with something
    int data_packet_size = params::get<int>("data_packet_size");
    message = (char *)malloc(data_packet_size);
    for (int i=0; i<data_packet_size-1; i++) {
      message[i] = '.';
    }
    message[data_packet_size-1] = '\0';

    payload = new string(message, data_packet_size);
    ASSERT(payload != NULL);
    maceLog("appmacedon: Allocated string at %p\n",payload);

    // Determine the ammount of time between each packet
    streaming_rate = params::get<int>("streaming_rate");
    packet_spacing = (double)(data_packet_size) *8/(1000.0*(double) streaming_rate);

    // Initialize the random seed
    double time_r = TimeUtil::timed();
    time_r += params::get<int>("streaming_time");

    if (params::containsKey("mode")) {
      std::string mode = params::get<std::string>("mode");
      if (mode == "RandTree") {
        params::set("ServiceConfig.Multicast", "GenericTreeMulticast");
        params::set("ServiceConfig.GenericTreeMulticast.tree_", "RandTree");
      }
      else if (mode == "ScribeMS") {
        params::set("ServiceConfig.Multicast", "GenericTreeMulticast");
        params::set("ServiceConfig.GenericTreeMulticast.data_", "CacheRecursiveOverlayRoute");
        params::set("ServiceConfig.GenericTreeMulticast.tree_", "ScribeMS");
        params::set("ServiceConfig.OverlayRouter", "Pastry");
        params::set("ServiceConfig.Group", "ScribeMS");
      }
      else if (mode == "ScribeMS-Bamboo") {
        params::set("ServiceConfig.Multicast", "GenericTreeMulticast");
        params::set("ServiceConfig.GenericTreeMulticast.data_", "CacheRecursiveOverlayRoute");
        params::set("ServiceConfig.GenericTreeMulticast.tree_", "ScribeMS");
        params::set("ServiceConfig.OverlayRouter", "Bamboo");
        params::set("ServiceConfig.Group", "ScribeMS");
      }
      else if (mode == "ScribeMS-Chord") {
        params::set("ServiceConfig.Multicast", "GenericTreeMulticast");
        params::set("ServiceConfig.GenericTreeMulticast.data_", "CacheRecursiveOverlayRoute");
        params::set("ServiceConfig.GenericTreeMulticast.tree_", "ScribeMS");
        params::set("ServiceConfig.OverlayRouter", "Chord");
        params::set("ServiceConfig.Group", "ScribeMS");
        params::set("ServiceConfig.Chord.FIX_FINGERS_EXPECTATION", "0");
      }
      else if (mode == "SplitStreamMS") {
        params::set("ServiceConfig.Multicast", "SplitStreamMS");
        params::set("ServiceConfig.Group", "SplitStreamMS");
        params::set("ServiceConfig.OverlayRouter", "Pastry");
      }
      else if (mode == "SplitStreamMS-Bamboo") {
        params::set("ServiceConfig.Multicast", "SplitStreamMS");
        params::set("ServiceConfig.Group", "SplitStreamMS");
        params::set("ServiceConfig.OverlayRouter", "Bamboo");
      }
      else if (mode == "SplitStreamMS-Chord") {
        params::set("ServiceConfig.Multicast", "SplitStreamMS");
        params::set("ServiceConfig.Group", "SplitStreamMS");
        params::set("ServiceConfig.OverlayRouter", "Chord");
        params::set("ServiceConfig.Chord.FIX_FINGERS_EXPECTATION", "0");
      }
    }

    globalMulticast = &(mace::ServiceConfig<MulticastServiceClass>::configure("appmacedon_test.multicast", StringSet(), StringSet(), false));
    if (params::containsKey("ServiceConfig.Group")) {
      //Assumes already created...  Not verified.
      GroupServiceClass* gp = &(mace::ServiceConfig<GroupServiceClass>::configure("appmacedon_test.group", StringSet(), StringSet(), false));
      groupServicesToJoin.push_back(gp);
    }

    if (globalRoute) {
      globalRoute->maceInit();
      thingsToExit.push_back(globalRoute);
      globalRoute->registerHandler(*appHandler, appHandler->uid);
      route_local_addr = globalRoute->localAddress();
    } 
    if (globalMulticast) {
      globalMulticast->maceInit();
      thingsToExit.push_back(globalMulticast);
      globalMulticast->registerHandler(*appHandler, appHandler->uid);
      multicast_local_addr = globalMulticast->localAddress();
    }

    local_ip = MaceKey(ipv4, Util::getMaceAddr());

    maceLog("local_ip: %s\n", local_ip.toString().c_str());

    if (local_ip == source) {  // I am the "root"
      printf("%s :: Automatically starting as sender.\n", appidstring.c_str());
      //       ANNOTATE_SET_PATH_ID_STR(NULL, 0, "start_sender-%s", local_ip.toString().c_str());
      start_sender(run_length, join_time, streaming_time);
    }
    else {
      if (app_stream && stream_type != RANDOM_HASH_ROUTE) {
        printf("%s :: Automatically starting as receiver.\n", appidstring.c_str());
        //         ANNOTATE_SET_PATH_ID_STR(NULL, 0, "start_receiver-%s", local_ip.toString().c_str());
        start_receiver(run_length, join_time, streaming_time);
      } else {
        printf("%s :: Automatically starting as sender.\n", appidstring.c_str());
        //         ANNOTATE_SET_PATH_ID_STR(NULL, 0, "start_sender-%s", local_ip.toString().c_str());
        start_sender(run_length, join_time, streaming_time);
      }
    }

    // Print receiver statistics.
    int window;
    if (gotten)
      window = gotten-1;
    else
      window = 0;
    double end = TimeUtil::timed();
    double bw = ((double)(window)* params::get<int>("data_packet_size")*8/(1000*(end-time_r)));
    if (params::get<double>("streaming_time") < run_length) {
      printf("%s :: REPLAY got %d pkts: avg bw %.2f Kbps time %lf avg lat %.8f\n", appidstring.c_str(), gotten, bw, end-time_r, avg_lat);
    } else {
      bw = streaming_rate;
    }
    printf("%s :: Exiting at time %lf\n", appidstring.c_str(), TimeUtil::timed());
    fflush(NULL);

    // All done.
    for (std::list<ServiceClass*>::iterator ex = thingsToExit.begin(); ex != thingsToExit.end(); ex++) {
      (*ex)->maceExit();
    }

    Accumulator::stopLogging();
    LoadMonitor::stopLoadMonitor();
    Scheduler::haltScheduler();

    int success = ! (bw >= streaming_rate/2);
    if (MACE_PORT == SOURCE_PORT) {
      success = 0;
      for (int i = 0; i < num_clients; i++) {
        int status = 0;
        printf("%s :: calling wait()\n", appidstring.c_str());
        ASSERTMSG(wait(&status) >= 0, "Wait() on a child process failed!");
        if (WIFEXITED(status)) {
          status = WEXITSTATUS(status);
        } else {
          status = 255;
        }
        printf("%s :: got status %d from waiting (success before or'ing %d)\n", appidstring.c_str(), status, success);
        success |= status;
      }
    }
    printf("%s :: Returning success = %d (0 is good)\n", appidstring.c_str(), success);

    return success;
    //   return 0;
  }
}


int 
stream_data(int run_length, int join_time, int streaming_time)
{
  ADD_FUNC_SELECTORS;
  Accumulator* sendCounter = Accumulator::Instance(Accumulator::APPLICATION_SEND);
  int pcount = 0;
  double start = TimeUtil::timed();
  double now = start;
  double prev_sent = now- packet_spacing;
  char qchar = 'A';
  int qcount=0;

  MaceKey unicastDest;
  if (app_stream && stream_type == UNICAST_DEST_ROUTE) {
    unicastDest = MaceKey(ipv4, params::get<std::string>("dest"));
  }

  //   int seq = 0;
  int data_packet_size = params::get<int>("data_packet_size");
  while (now - start < run_length - streaming_time) {
    ASSERT(payload != NULL);
    *(double *)payload->data() = TimeUtil::timed();
    int mindex = pcount/26 % (data_packet_size-sizeof(double));
    if (mindex == 0) {
      qchar = (char) ((int) qchar + qcount%26 );
      qcount ++;
    }
    char pchar = (char)( (int)'A' + pcount%26 );
    (*payload)[mindex+sizeof(double)] = pchar;
    if(app_stream) {
      if(stream_type == RANDOM_HASH_ROUTE) {
        uint32_t bytes[5];
        bytes[0] = RandomUtil::randInt();
        bytes[1] = RandomUtil::randInt();
        bytes[2] = RandomUtil::randInt();
        bytes[3] = RandomUtil::randInt();
        bytes[4] = RandomUtil::randInt();
        MaceKey dest(sha160, bytes);
        macedbg(1) << "routing " << payload->size() << " bytes from " << route_local_addr << " to " << dest << Log::endl;
        globalRoute->route(dest, *payload, appHandler->uid);
      } else if(stream_type == UNICAST_DEST_ROUTE) {
        macedbg(1) << "routing " << payload->size() << " bytes from " << route_local_addr << " to " << unicastDest << Log::endl;
        globalRoute->route(unicastDest, *payload, appHandler->uid);
      } else if(stream_type == GROUP_MULTICAST) {
        macedbg(1) << "multicasting " << payload->size() << " bytes from " << multicast_local_addr << " on group " << GROUPID << Log::endl;
        //         ANNOTATE_SET_PATH_ID_STR(NULL, 0, "multicast-%d", seq++);
        globalMulticast->multicast(GROUPID, *payload, appHandler->uid);
      }

      maceDebug(2, "\"now\" %lf diff %lf expected spacing %lf streamrate %d, actual %f, pcount %d, timespent %f\n", now, now-prev_sent, packet_spacing, streaming_rate, ((double)(pcount-1)*(data_packet_size<<3)/(1000*(now-start))), pcount, now-start);
      pcount++;
      sendCounter->accumulate(data_packet_size<<3);
    }
    prev_sent = now;
    message[mindex] = qchar;
    now = TimeUtil::timed();
    double time_sending = now - start;
    // figure out how many pkts we should have sent
    int should_have_sent = (int)(time_sending / packet_spacing);
    if (pcount >= should_have_sent) {
      // only sleep if we have sent what we should have
      SysUtil::sleepu((uint64_t)(packet_spacing*1000000));
    }
    now = TimeUtil::timed();
  }
  
  return pcount;
}

void
start_sender(int run_length, int join_time, int streaming_time)
{
  ADD_FUNC_SELECTORS;
  int pcount = 0;

  maceout << "run_length: " << run_length << " join_time: " << join_time << " streaming_time: " << streaming_time << Log::endl;

  printf("%s :: appmacedon: Sending packets every %f seconds.\n", appidstring.c_str(), packet_spacing);

  if(streaming_time < run_length) {
    
    //Sleep until streaming_time
    maceout << "Sleeping for " << join_time << " seconds until join_time" << Log::endl;
    if (join_time) {
      SysUtil::sleep(join_time);
    }
    maceout << "Done sleeping for " << join_time << " seconds until join_time" << Log::endl;
    //Creating the groups
    for(std::deque<GroupServiceClass*>::iterator i = groupServicesToJoin.begin(); i != groupServicesToJoin.end(); i++) {
      //       ANNOTATE_SET_PATH_ID_STR(NULL, 0, "createGroup-%s-%p", local_ip.toString().c_str(), *i);
      maceout << "Creating group " << GROUPID << " on pointer " << (unsigned long)*i << Log::endl;
      (**i).createGroup(GROUPID, appHandler->uid);
      maceout << "Done creating group " << GROUPID << " on pointer " << (unsigned long)*i << Log::endl;
    }
    //Sleep until streaming_time
    maceout << "Sleeping for " << (streaming_time-join_time) << " seconds until streaming_time" << Log::endl;
    if (streaming_time-join_time) {
      SysUtil::sleep(streaming_time-join_time);
    }
    maceout << "Done sleeping for " << (streaming_time-join_time) << " seconds until streaming_time" << Log::endl;
    receiver_started=true;
    double start = TimeUtil::timed();

    pcount = stream_data(run_length, join_time, streaming_time);
    double end = TimeUtil::timed();
    if (pcount==0) {
      pcount=1;
    }
    printf("%s :: REPLAY sent %d pkts: avg bw %.2f Kbps time %lf\n",appidstring.c_str(), pcount, ((double)(pcount-1)*(params::get<int>("data_packet_size")) *8/(1000*(end-start))), end-start);
  } else if (run_length) {
    SysUtil::sleep(run_length);
  }
  
  return;
}


void
start_receiver(int run_length, int join_time, int streaming_time)
{
  ADD_FUNC_SELECTORS;
  if(streaming_time < run_length) {
    if (join_time) {
      SysUtil::sleep(join_time);
    }
    for(std::deque<GroupServiceClass*>::iterator i = groupServicesToJoin.begin(); i != groupServicesToJoin.end(); i++) {
      //       ANNOTATE_SET_PATH_ID_STR(NULL, 0, "joinGroup-%s-%p", local_ip.toString().c_str(), *i);
      maceout << "Joining group " << GROUPID << " on poitner " << (unsigned long)*i << Log::endl;
      (**i).joinGroup(GROUPID);
    }
    if (streaming_time-join_time) {
      SysUtil::sleep(streaming_time-join_time);
    }

    receiver_started=true;
    if (run_length - streaming_time) {
      SysUtil::sleep(run_length-streaming_time);  // wait til the end of the run
    }
  } else if (run_length) {
    SysUtil::sleep(run_length);  // wait til the end of the run
  }

  return;
}



