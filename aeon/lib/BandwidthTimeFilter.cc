/* 
 * BandwidthTimeFilter.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
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
// #include <stdio.h>
#include "BandwidthTimeFilter.h"
#include "TimeUtil.h"
#include "massert.h"
// #include "mace.h"

using std::list;

const double BandwidthTimeFilter::DEFAULT_BANDWIDTH_WINDOW = 2.5;

BandwidthTimeFilter::BandwidthTimeFilter(double wnd) : window(wnd), bandwidth(0.0) {
}

BandwidthTimeFilter::~BandwidthTimeFilter() {
}

double BandwidthTimeFilter::getValue() {
  check();
  return bandwidth;
}

void BandwidthTimeFilter::clear() {
  history.clear();
  bandwidth = 0.0;
}

void BandwidthTimeFilter::startUpdate() {
  // techinically we could start two updates without finishing one,
  // so be careful and don't do this
  double now =  TimeUtil::timed();

  // just add an entry with the start time
  if(!history.empty() && history.back().stop != 0 && history.back().size != 0) { 
    // This is a hack to add a "finish" block if we happen to start a segment
    // without finishing one.  This can happen if we add source as a neighbor
    // later on when we were recording bandwidth to him without having
    // requested anything.
    history.push_back(BandwidthTriplet(0, history.back().stop, 0));
  }
  history.push_back (BandwidthTriplet (now, 0, 0));
}

void BandwidthTimeFilter::update(int size) {
  double now = TimeUtil::timed();

  history.push_back(BandwidthTriplet(0, now, size));
}

void BandwidthTimeFilter::finishUpdate() {
  double now =  TimeUtil::timed();

  history.push_back(BandwidthTriplet(0, now, 0));
}

void BandwidthTimeFilter::check() {
  double elapsed_time = 0;
  double stop_time = -1;
  double end = 0;
  int total_bytes = 0;
  double seg_start, seg_stop = 0.0;

  if(history.size() <= 1) {
    bandwidth = 0.0;
    //    printf("Returning because not enough bw filter entries\n");
    return;
  }

  for(list<BandwidthTriplet>::reverse_iterator it = history.rbegin();
      it != history.rend(); ++it) {
    //printf("Entry is (%f %f %d)\n", it->start, it->stop, it->size);
    if(it->stop > 0 && end == 0) {
      /*if(Util::timed() - it->stop >= window) {
      //printf("First entry is outside bandwidth window\n");
      bandwidth = 0.0;
      return;
      }*/
      end = it->stop;
      seg_stop = end;
      //printf("End time is %f\n", end);
    }
    if(it->stop > 0) {
      total_bytes += it->size;
      if(it->size == 0)
        seg_stop = it->stop;
      if(end - it->stop >= window) {
        stop_time = it->stop;
        elapsed_time += (seg_stop - it->stop);
        break;
      }

    }
    else if(end != 0) {
      ASSERTMSG(it->start != 0, "Segment added, but start is non zero");
      seg_start = it->start;
      //printf("A segment from %f to %f has been added\n", seg_start, seg_stop);
      elapsed_time += (seg_stop - seg_start);
      if(elapsed_time >= window) {
        stop_time = it->start;
        break;
      }
    }
  }
  //  if(stop_time == -1)
  //  stop_time = (history.front().stop > 0 ? history.front().stop : history.front().start);
  if(stop_time != -1) {
    //printf("Stop time is %f\n", stop_time);
    while((history.front().start > 0 && history.front().start != stop_time)
        || (history.front().stop > 0 && history.front().stop != stop_time)) {
      history.pop_front();
    }
    if(history.front().start == 0) {
      history.front().start = history.front().stop;
      history.front().stop = 0;
      history.front().size = 0;
    }
  }
  if(history.front().start == 0) {
    history.front().start = history.front().stop;
    history.front().stop = 0;
    history.front().size = 0;
  }
  bandwidth = total_bytes / elapsed_time;
  //printf("Counted %d bytes over %f time. BW = %f\n", total_bytes, elapsed_time, bandwidth);

}
