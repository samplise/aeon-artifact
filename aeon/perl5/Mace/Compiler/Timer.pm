# 
# Timer.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, James W. Anderson, Adolfo Rodriguez, Dejan Kostic
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the names of the contributors, nor their associated universities 
#      or organizations may be used to endorse or promote products derived from
#      this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# ----END-OF-LEGAL-STUFF----
package Mace::Compiler::Timer;

use strict;

use Mace::Compiler::Type;
use Mace::Util qw(:all);

use Class::MakeMethods::Template::Hash 
    (
     'new --and_then_init' => 'new',
     'string' => "name",
     'array_of_objects' => ["types" => { class => "Mace::Compiler::Type"}],

     'string' => "scheduleSelector",
     'string' => "cancelSelector",
     'string' => "expireSelector",

     'string' => "expireMethod",

      #from type options
     'scalar' => "recur",
     'boolean' => "dump",
     'boolean' => "serialize",
     'boolean' => "multi", #implies the same timer may be scheduled more than once
     'boolean' => "random",
     'scalar' => "trace",
     'scalar' => "simWeight",

      #for error messages
     'string' => 'filename',
     'number' => 'line',

     'array_of_objects' => ["params" => { class => "Mace::Compiler::Param"}],
     'array_of_objects' => ["typeOptions" => { class => "Mace::Compiler::TypeOption" }],
     'boolean' => "shouldLog",
     'string' => "logClause",

      #for context timer check
     'boolean' => "isContextTimer",
     );

sub init {
    my $this = shift;
    unless ($this->expireMethod()) {
        $this->expireMethod('expire_'.$this->name());
    }
}

sub type {
  my $this = shift;
  return Mace::Compiler::Type->new(type=>$this->name."_MaceTimer");
}

sub toString {
    my $this = shift;
    my $name = shift;
    my %args = @_;
    my $r = "";
    my $n = $this->name();
    my $maptype = "mace::map<uint64_t, TimerData*, mace::SoftState>";
    my $keytype = "uint64_t";
    my $valuetype = "TimerData*";
    my $scheduleFields = "";
#     my $noCommaScheduleFields = "";
    my $params = "";
    my $commaparams = "";
    my $timerDataBody = "";
    my $callParams = "";
    my $commaCallParams = "";
    my $traceLevel = $this->getLogLevel($args{traceLevel});
    my $lockType = $args{locktype};

    my $traceg1 = ($traceLevel > 1)?'true':'false';
    my $trace = ($traceLevel > 0 && $this->shouldLog())?'true':'false';
    my $printNodeFields = "";
    my $printFields = "";
    my $printFieldState = "";
    my $serializeFields = "";
    my $deserializeFields = "";
#     my $maceoutPrintFields = "";
#     my $maceoutPrintFieldsTd = "";

    my $macetime = $Mace::Compiler::Globals::MACE_TIME;
    my $prep = "PREPARE_FUNCTION";
    my $expirePrep = "ANNOTATE_SET_PATH_ID(NULL, 0, timerData->pipPathId, timerData->pipPathIdLen);";
    my $timeType = "uint64_t";
    my $realtime = "";
    if ($macetime) {
#	$prep = "MaceTime curtime = MaceTime::currentTime();";
	$timeType = "MaceTime";
	$realtime = ".time()";
    }

#     my $compareFields = "";

    $timerDataBody .= qq/
      public:
      char* pipPathId;
      int pipPathIdLen;
      TimerData() : pipPathId(NULL), pipPathIdLen(0) { }
      ~TimerData() { ::free(pipPathId); }
    /;

    my $zeroTimerData = $this->multi() ? "" : ", timerData(0)";

    if ($this->count_params()) {
      $scheduleFields = join("", map{", ".$_->toString(paramconst => 1, paramref => 1)} $this->params());
      $params = join(", ", map{"temptd->".$_->name()} $this->params());
      $commaparams = ", $params";
      $timerDataBody .= ("TimerData(" .
			 join(", ", map{$_->toString(paramconst => 1, paramref=> 1)}
			      $this->params()) .") : " .
			 join(", ", map{$_->name()."(".$_->name().")"} $this->params()) .
			 "{ }\n");
      $timerDataBody .= join("\n", map{$_->toString().";"} $this->params());
      $callParams = join(", ", map{$_->name()} $this->params());
      $commaCallParams = ", $callParams";
      $printNodeFields = join("\n", map{qq/mace::printItem(__pr, "${\$_->name()}", &td->${\$_->name()});/} $this->params());
      $printFields = join("", map{qq/", ${\$_->name()}="; mace::printItem(__out, &td->${\$_->name()}); __out << /} $this->params());
      $serializeFields = join("", map{qq/mace::serialize(__str, &td->${\$_->name()});/} $this->params());
      $deserializeFields = join("", map{qq/serializedByteSize += mace::deserialize(__in, &${\$_->name()});/} $this->params());
      $printFieldState = join("", map{qq/", ${\$_->name()}="; mace::printState(__out, &td->${\$_->name()}, td->${\$_->name()}); __out << /} $this->params());
    }

    my $schedulePrep = qq{
	timerData = new TimerData($callParams);
	timerData->pipPathId = annotate_get_path_id_copy(&timerData->pipPathIdLen);
    };

    my $printNodeBody = qq/
      TimerData* td __attribute((unused)) = timerData;
      mace::PrintNode __pr("${\$this->name()}", "timer");
      mace::printItem(__pr, "scheduled", &nextScheduledTime);
      if (nextScheduledTime) {
        $printNodeFields
      }
      __printer.addChild(__pr);
    /;
    my $printBody = qq/
      if (nextScheduledTime) {
        TimerData* td __attribute((unused)) = timerData;
        __out << "timer<${\$this->name}>(scheduled=" << nextScheduledTime << $printFields ")";
      }
      else {
        __out << "timer<${\$this->name}>(not scheduled)";   
      }
    /;
    my $printStateBody = qq/
      if (nextScheduledTime) {
        TimerData* td __attribute((unused)) = timerData;
        __out << "timer<${\$this->name}>(scheduled" << $printFieldState")";
      } 
      else {
        __out << "timer<${\$this->name}>(not scheduled)";   
      }
    /;
    
    my $serializeBody = qq/
      if(nextScheduledTime) {
        mace::serialize(__str, &nextScheduledTime);
      } else {
        uint64_t dummy = 0; 
        mace::serialize(__str, &dummy);
      }
    /;
    my $deserializeBody = qq/
      return mace::deserialize(__in, &nextScheduledTime);
    /;

    my $timerDataObject = ($this->multi()?"$maptype timerData":"TimerData *timerData");
    my $multiIsScheduled = "";
    my $nextScheduled = "return nextScheduledTime";
    my $nextScheduledTime = "uint64_t nextScheduledTime;";
    my $nextScheduledConstructor = ", nextScheduledTime()";
    my $rescheduleMethod = "$timeType reschedule(const $timeType& interval $scheduleFields) {
      cancel();
      return schedule(interval $commaCallParams);
    }";
    my $expireMethodName = $this->expireMethod();
    my $scheduleMethodName = "scheduler_" . $this->name();
    my $scheduleBody = qq{
      ASSERTMSG(!isScheduled(), Attempting to schedule an already scheduled timer: ${\$this->name});
      nextScheduledTime = TimerHandler::schedule(interval$realtime, false);
      return nextScheduledTime;
      };
    my $multiCancel = "";
    my $multiNumScheduled = "";
    my $cancelMethod = qq{
      if (TimerHandler::isRunning()) {
	TimerHandler::cancel();
	nextScheduledTime = 0; 
	delete timerData;
	timerData = 0;
      }
    };
    my $fireTimerHandler = "";
    $fireTimerHandler = qq#
    // chuangw: context'ed timer sends a message to header  similar to async_foo() helper call
    agent_->$scheduleMethodName($params);#;
    my $expireFunction = qq{
      TimerData* temptd = timerData; 
      timerData = 0;
      $fireTimerHandler

      delete temptd;
    };

    if($this->multi()) {
        $multiNumScheduled = "size_t scheduledCount() const { return timerData.size(); }";
        $multiIsScheduled = "bool isScheduled(const $timeType& expireTime) const { return timerData.containsKey(expireTime$realtime); }";
        $nextScheduled = "return timerData.empty()?0:timerData.begin()->first";
        $nextScheduledTime = "";
        $schedulePrep = qq{
            TimerData* td = new TimerData($callParams);
            td->pipPathId = annotate_get_path_id_copy(&td->pipPathIdLen);
        };
        $expirePrep = "";
        $nextScheduledConstructor = "";
        $printNodeBody = qq/
            mace::PrintNode __tpr("${\$this->name()}", "timer");
            size_t pos = 0;
            for (${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
                mace::PrintNode __pr(StrUtil::toString(pos), "TimerData");
                TimerData* td __attribute((unused)) = i->second;
                mace::printItem(__pr, "scheduled", &(i->first));
                $printNodeFields
                pos++;
                __tpr.addChild(__pr);
            }
            __printer.addChild(__tpr);
        /;
        $printBody = qq/
            __out << "timer<${\$this->name}>(";
            for(${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
              TimerData* td __attribute((unused)) = i->second;
              __out << "[scheduled=" << i->first << $printFields "]";
            }
            __out << ")";
        /;
        $printStateBody = qq/
            __out << "timer<${\$this->name}>(";
            for(${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
                TimerData* td __attribute((unused)) = i->second;
                __out << "[scheduled" << $printFieldState "]";
            }
            __out << ")";
        /;
        $serializeBody = qq/
              uint32_t sz = timerData.size();
              mace::serialize(__str, &sz);
              for(${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
                $keytype key = i->first;
                mace::serialize(__str, &key);
                TimerData* td __attribute((unused)) = i->second;
                $serializeFields
              }
        /;
        
        my $timerDataVariables = join("\n", map{$_->toString().";"} $this->params());
        $deserializeBody = qq/
              int serializedByteSize = 0;
              uint32_t sz;
              serializedByteSize += sizeof(sz);
              mace::deserialize(__in, &sz);

              for(size_t i = 0; i < sz; i++) {
                $keytype key;
                serializedByteSize += mace::deserialize(__in, &key);
                $timerDataVariables;
                $deserializeFields
                TimerData* td = new TimerData($callParams);
                timerData[key] = td;
              }
              return serializedByteSize;
        /;

        my $random = $this->random() . " && Scheduler::simulated()";
        $scheduleBody = qq|
    //	  TimerData* td = new TimerData($callParams);
          uint64_t t = mace::getmtime() + interval$realtime;
          if ($random) {
            t = mace::getmtime();
          }
          // maceout << "interval=" << interval << " t=" << t << Log::endl;
          ${maptype}::iterator i = timerData.find(t);
          while(i != timerData.end() && i->first == t) {
            // maceout << "found duplicate timer scheduled for " << t << Log::endl;
            i++;
            t++;
          }
    // 	  std::cerr << "assigning timer to " << t << std::endl;
          timerData[t] = td;
          if (timerData.begin()->first == t) {
            TimerHandler::cancel();
                if ($trace) { maceout << "calling schedule for " << t << Log::endl; }
            ASSERT(TimerHandler::schedule(t, true) == t);
          }
          return t;
        |;

        $multiCancel = qq/
          void cancel(const $timeType& expireTime) {
                #define selector selector_cancel$n
                #define selectorId selectorId_cancel$n

            $prep
            ADD_LOG_BACKING
            ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1, $trace && mace::LogicalClock::instance().shouldLogPath(), PIP);

                if ($trace) { maceout << "canceling timer at " << expireTime << Log::endl; }

            ${maptype}::iterator i = timerData.find(expireTime$realtime);
            if (i == timerData.end()) {
                  if ($trace) { maceout << "timer not found, returning" << Log::endl; }
              return;
            }
            delete i->second;
            if (i == timerData.begin()) {
                  if ($trace) { maceout << "calling TimerHandler::cancel on" << getId() << Log::endl; }
              TimerHandler::cancel();
              timerData.erase(i);
              if (!timerData.empty()) {
                    if ($trace) { maceout << "calling schedule for " << timerData.begin()->first << Log::endl; }
            TimerHandler::schedule(timerData.begin()->first, true);
              }
            }
            else {
                  if ($trace) { maceout << "erasing from timerData " << i->first << Log::endl; }
              timerData.erase(i);
            }

            #undef selector
            #undef selectorId
          }
                   /;

        if ($this->count_params()) {
            my $nparam = $this->count_params();
            for my $i (0..($nparam - 1)) {
            my @param = $this->params();
            @param = @param[0..$i];
            my $noCommaScheduleFields = join(", ", map { $_->toString(paramconst => 1, paramref => 1) } @param);

            my $compareFields = join(" && ", map{"(temptd->".$_->name()." == ".$_->name().")"} @param);

            my $maceoutPrintFields = join("", map{qq/", ${\$_->name()}="; mace::printItem(maceout, &${\$_->name()}); maceout << /} @param);
            my $maceoutPrintFieldsTd = join("", map{qq/", ${\$_->name()}="; mace::printItem(maceout, &temptd->${\$_->name()}); maceout << /} $this->params());

            $multiCancel .= qq/
    void cancel($noCommaScheduleFields) {
      #define selector selector_cancel$n
      #define selectorId selectorId_cancel$n
      $prep
      ADD_LOG_BACKING
    ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1, $trace && mace::LogicalClock::instance().shouldLogPath(), PIP);
      
      if ($trace) { maceout << "canceling timer for " << $maceoutPrintFields Log::endl; }
      
      ${maptype}::iterator i = timerData.begin();
      bool reschedule = false;
      while (i != timerData.end()) {
        TimerData* temptd = i->second;
        if (true && $compareFields) {
          if ($trace) { maceout << "erasing " << i->first << " : " << $maceoutPrintFieldsTd Log::endl; }
          delete i->second;
          timerData.erase(i++);
          reschedule = true;
        }
        else {
          i++;
        }
      }
      if (reschedule) {
        if ($trace) { maceout << "calling TimerHandler::cancel on " << getId() << Log::endl; }
        TimerHandler::cancel();
        if(!timerData.empty()) {
          if ($trace) { maceout << "calling schedule for " << timerData.begin()->first << Log::endl; }
          TimerHandler::schedule(timerData.begin()->first, true);
        }
      }

    #undef selector
    #undef selectorId
    }
                   
            /;
            
            }
        }
        
            
        $cancelMethod = qq|TimerHandler::cancel();
                           maceout << "canceling all timers" << Log::endl;
                           for(${maptype}::iterator i = timerData.begin(); i != timerData.end(); i++) {
                             delete i->second;
                           }
                           // maceout << "clearing timerData" << Log::endl;
                           timerData.clear();
            |;
        $rescheduleMethod = "";
        
        my $loopCondition = "(i->first < (curtime + Scheduler::CLOCK_RESOLUTION))";
            my $weightsTrue = "";
            my $weightsFalse = "";
        if ($macetime) {
            $loopCondition = "MaceTime(i->first - Scheduler::CLOCK_RESOLUTION).lessThan(curtime, trueWeight, falseWeight)";
                $weightsTrue = qq/ int trueWeight = 1;
                                   int falseWeight = 0; /;
                $weightsFalse = qq/ trueWeight = 0;
                                    falseWeight = 1; /;
        }

        $expireFunction = qq/
        ASSERT(!timerData.empty());
        if ($random) {
          unsigned ntimers = RandomUtil::randInt(timerData.size()) + 1;
              if ($trace) { maceout << "firing " << ntimers << " random timers" << Log::endl; }
          std::vector<std::pair<uint64_t, TimerData*> > toFire;
          $maptype copy = timerData;
          for (unsigned i = 0; i < ntimers; i++) {
            unsigned which = RandomUtil::randInt(copy.size());
            ${maptype}::iterator mi = copy.begin();
            for (unsigned j = 0; j < which; j++) {
              mi++;
            }
            toFire.push_back(*mi);
                if ($trace) { maceout << "selecting " << mi->first << Log::endl; }
            copy.erase(mi);
          }
          for (std::vector<std::pair<uint64_t, TimerData*> >::iterator i = toFire.begin();
               i != toFire.end(); i++) {
            if (timerData.containsKey(i->first)) {
              TimerData* temptd = i->second;
              ANNOTATE_SET_PATH_ID(NULL, 0, temptd->pipPathId, temptd->pipPathIdLen);
              ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1, $trace && mace::LogicalClock::instance().shouldLogPath(), PIP);
              timerData.erase(i->first);
                  if ($trace) { maceout << "firing " << i->first << Log::endl; }
              $fireTimerHandler
              delete temptd;
            }
            else {
                  if ($trace) { maceout << i->first << " canceled!" << Log::endl; }
            }
          }
        }
        else {
          ${maptype}::iterator i = timerData.begin();
              $weightsTrue
          while((i != timerData.end()) && $loopCondition) {
            TimerData* temptd = i->second;
            timerData.erase(i);
                if ($trace) { macecompiler(0) << "calling expire into service for $n " << i->first <<  Log::endl; }
            $fireTimerHandler
            delete temptd;
            i = timerData.begin();
                $weightsFalse
          }
        }
        if (!timerData.empty() && !TimerHandler::isScheduled()) {
          if ($trace) { maceout << "calling resched for " << timerData.begin()->first << Log::endl; }
          TimerHandler::schedule(timerData.begin()->first, true);
        }
        else if (timerData.empty() && TimerHandler::isScheduled()) {
              if ($trace) { maceout << "canceling already expired timer" <<Log::endl; }
          TimerHandler::cancel();
        }
          /;
    } # if($this->multi())
    my $addDefer = "";
    if ($traceLevel > 2) {
        $addDefer = "mace::ScopedStackExecution::addDefer(agent_);
        ";
    }
    my $serializeMethods = "";
    if ($this->serialize()) {
	$serializeMethods = qq/
              void serialize(std::string& __str) const {
                $serializeBody
              }
              int deserialize(std::istream& __in) {
                $deserializeBody
              }
/;
    }

    my $simWeight = $this->simWeight();
    
    $r .= qq@class ${name}::${n}_MaceTimer : private TimerHandler, public mace::PrintPrintable {
            public:
              ${n}_MaceTimer($name *a)
                : TimerHandler("${name}::${n}", $simWeight), agent_(a) $nextScheduledConstructor $zeroTimerData
              {
	      }

	      virtual ~${n}_MaceTimer() {
		cancel();
	      }
              
              bool isScheduled() const {
                return TimerHandler::isScheduled() || TimerHandler::isRunning();
              }

              $timeType schedule(const $timeType& interval $scheduleFields) {
                #define selector selector_schedule$n
                #define selectorId selectorId_schedule$n
                $prep
                $schedulePrep
                ADD_LOG_BACKING
                ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1, $trace && mace::LogicalClock::instance().shouldLogPath(), PIP);

                $scheduleBody
                #undef selector
                #undef selectorId
              }
              $rescheduleMethod
              void cancel() {
                #define selector selector_cancel$n
                #define selectorId selectorId_cancel$n
                $prep
                ADD_LOG_BACKING
                ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1, $trace && mace::LogicalClock::instance().shouldLogPath(), PIP);
                $cancelMethod
                #undef selector
                #undef selectorId
              }
              $multiCancel
              $timeType nextScheduled() const { 
                $nextScheduled;
              }
              $multiIsScheduled
	      $multiNumScheduled

              void printNode(mace::PrintNode& __printer, const std::string& __name) const {
                $printNodeBody
              }
              void print(std::ostream& __out) const {
                $printBody
              }
              void printState(std::ostream& __out) const {
                $printStateBody
              }
              $serializeMethods

	    protected:
              void expire() {
                #define selector selector_expire$n
                #define selectorId selectorId_expire$n
                $prep
                $expirePrep
                ADD_LOG_BACKING
        // chuangw: temporary hack. timer handler needs to use the ticket.
        //mace::AgentLock::skipTicket();
        //macedbg(1)<<"ticket = "<< ThreadStructure::myTicket() <<Log::endl;
        mace::ScopedFingerprint __fingerprint(selector);
                mace::ScopedStackExecution __defer;
                $addDefer
                ScopedLog __scopedLog(selector, 0, selectorId->compiler, true, $traceg1,
				      $trace && mace::LogicalClock::instance().shouldLogPath(), PIP && !${\$this->multi()});

                if (!isRunning()) {
                  return;
                }

                clearRunning();

                $expireFunction
                #undef selector
                #undef selectorId
              }
              
              
            protected:
              $name *agent_;
              $nextScheduledTime

              class TimerData {
                $timerDataBody
              };

              $timerDataObject;
            };
          @;
    return $r;
} # toString

sub toStringDummy{
    my $this = shift;
    my $name = shift;
    my $callParams = "";
    my $n = $this->name();
    my $maptype = "mace::map<uint64_t, TimerData*, mace::SoftState>";
    my $keytype = "uint64_t";
    my $valuetype = "TimerData*";
    my $r = "";
    my $printFields = "";
    my $printFieldState = "";
    my $deserializeFields = "";
    my $timerDataBody = "";

    $timerDataBody .= qq/
      public:
      char* pipPathId;
      int pipPathIdLen;
      TimerData() : pipPathId(NULL), pipPathIdLen(0) { }
      ~TimerData() { ::free(pipPathId); }
    /;

    if($this->count_params()) {
      $callParams = join(", ", map{$_->name()} $this->params());
      $timerDataBody .= ("TimerData(" .
                         join(", ", map{$_->toString(paramconst => 1, paramref=> 1)}
                              $this->params()) .") : " .
                         join(", ", map{$_->name()."(".$_->name().")"} $this->params()) .
                         "{ }\n");
      $timerDataBody .= join("\n", map{$_->toString().";"} $this->params());
      $printFields = join("", map{qq/", ${\$_->name()}="; mace::printItem(__out, &td->${\$_->name()}); __out << /} $this->params());
      $deserializeFields = join("", map{qq/serializedByteSize += mace::deserialize(__in, &${\$_->name()});/} $this->params());
      $printFieldState = join("", map{qq/", ${\$_->name()}="; mace::printState(__out, &td->${\$_->name()}, td->${\$_->name()}); __out << /} $this->params())
    }

    my $printBody = qq/
      TimerData* td __attribute((unused)) = timerData;
      __out << "timer<${\$this->name}>(scheduled=" << nextScheduledTime << $printFields ")";
    /;
    my $printStateBody = qq/
      if(nextScheduledTime) {
        TimerData* td __attribute((unused)) = timerData;
        __out << "timer<${\$this->name}>(scheduled" << $printFieldState")";
      } else {
        __out << "timer<${\$this->name}>(not scheduled)";
      }
    /;
   
    my $serializeBody = qq/
      if(nextScheduledTime) {
        mace::serialize(__str, &nextScheduledTime);
      } else {
        uint64_t dummy = 0;
        mace::serialize(__str, &dummy);
      }
    /;
    my $deserializeBody = qq/
      return mace::deserialize(__in, &nextScheduledTime);
    /;

    my $timerDataObject = ($this->multi()?"$maptype timerData":"TimerData *timerData");
    my $nextScheduledTime = "uint64_t nextScheduledTime;";
    my $nextScheduledConstructor = ", nextScheduledTime()";

    if($this->multi()) {
        $nextScheduledTime = "";
        $nextScheduledConstructor = "";
        $printBody = qq/
        __out << "timer<${\$this->name}>(";
        for(${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
          TimerData* td __attribute((unused)) = i->second;
          __out << "[scheduled=" << i->first << $printFields "]";
        }
        __out << ")";
        /;
        $printStateBody = qq/
            __out << "timer<${\$this->name}>(";
        for(${maptype}::const_iterator i = timerData.begin(); i != timerData.end(); i++) {
            TimerData* td __attribute((unused)) = i->second;
            __out << "[scheduled" << $printFieldState "]";
        }
        __out << ")";
        /;

        my $timerDataVariables = join("\n", map{$_->toString().";"} $this->params());
        $deserializeBody = qq/
          int serializedByteSize = 0;
          uint32_t sz;
          serializedByteSize += sizeof(sz);
          mace::deserialize(__in, &sz);

          for(size_t i = 0; i < sz; i++) {
            $keytype key;
            serializedByteSize += mace::deserialize(__in, &key);
            $timerDataVariables;
            $deserializeFields
            TimerData* td = new TimerData($callParams);
            timerData[key] = td;
          }
          return serializedByteSize;
        /;
    }

    my $deserializeMethod = "";
    if ($this->serialize()) {
	$deserializeMethod = qq/
              int deserialize(std::istream& __in) {
                $deserializeBody
              }
/;
    }
    
    my $simWeight = $this->simWeight();

    $r .= qq/class ${name}::${n}_MaceTimer : private TimerHandler, public mace::PrintPrintable {
            public:
              ${n}_MaceTimer($name *a)
                : TimerHandler("${name}::${n}", $simWeight), agent_(a)$nextScheduledConstructor
              {
              }

              virtual ~${n}_MaceTimer() {
                cancel();
              }

              bool isScheduled() const {
                return TimerHandler::isScheduled() || TimerHandler::isRunning();
              }

              void print(std::ostream& __out) const {
                $printBody
              }
              void printState(std::ostream& __out) const {
                $printStateBody
              }
              $deserializeMethod

            protected:
              $name *agent_;
              $nextScheduledTime
              void expire(){};

              class TimerData {
                $timerDataBody
              };

              $timerDataObject;
            };
          /;
    return $r;
} # toStringDummy


                              #uint sched_count_$t;
                              #void* pip_path_id_$t;
                              #int pip_path_len_$t;
                              #,\nsched_count_$timer(), pip_path_id_$timer(), pip_path_len_$timer()
sub validateTypeOptions {
    my $this = shift;
    my $sv = shift;
    my $name;
    my $value;
    my %fieldoptions;
    $this->dump(1);
    $this->serialize(1);
    $this->multi(0);
    $this->recur(0);
    $this->trace(-2);
    $this->simWeight(1);
    foreach my $option ($this->typeOptions()) {
	Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Cannot define option ".$option->name()." for field ".$this->name()." more than once!") unless(++$fieldoptions{$option->name()} == 1);

	if ($option->name() eq 'dump') {
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option dump must have exactly one sub-option") unless (scalar(keys(%{$option->options()})) == 1);
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'no') {
		    $this->dump(0);
		}
		elsif (! $name eq 'yes') {
		    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option with name $name and value $value");
		}
	    }
	}
        elsif ($option->name() eq 'serialize') {
            Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option serialize must have exactly one sub-option") unless (scalar(keys(%{$option->options()})) == 1);
            while (($name, $value) = each(%{$option->options()})) {
                if ($name eq 'no') {
                    $this->serialize(0);
                }
                elsif (! $name eq 'yes') {
                    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option with name $name and value $value");
                }
            }
        }
	elsif ($option->name() eq 'recur') {
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option recur must have exactly one sub-option") unless(scalar(keys(%{$option->options()})) == 1);
	    $this->recur((keys(%{$option->options()}))[0]);
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "options multi and recur cannot be used together") if($this->recur() and $this->multi());
	}
        elsif ($option->name() eq 'trace') {
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option recur must have exactly one sub-option") unless(scalar(keys(%{$option->options()})) == 1);
            if ((keys(%{$option->options()}))[0] eq 'off') {
                $this->trace(-1);
            }
            elsif ((keys(%{$option->options()}))[0] eq 'manual') {
                $this->trace(0);
            }
            elsif ((keys(%{$option->options()}))[0] eq 'low') {
                $this->trace(1);
            }
            elsif ((keys(%{$option->options()}))[0] eq 'med') {
                $this->trace(2);
            }
            elsif ((keys(%{$option->options()}))[0] eq 'high') {
                $this->trace(3);
            }
            else {
                Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option 'trace' must have parameter off, manual, low, med, or high");
            }

        }
	elsif ($option->name() eq 'multi') {
# 	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option multi must have exactly one sub-option") unless(scalar(keys(%{$option->options()})) == 1);
	    $this->random(0);
	    if (defined($option->options()->{"random"})) {
		if ($option->options("random") =~ m/^(random|yes)$/) {
		    $this->random(1);
		}
		elsif ($option->options("random") !~ m/^no$/) {
		    Mace::Compiler::Globals::error('bad_type_option', $option->line(),
						   "random cannot have value " .
						   $option->options("random"));
		}
		delete($option->options()->{"random"});
	    }

	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'yes') {
		    $this->multi(1);
		}
		elsif (! $name eq 'no') {
		    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option with name $name and value $value");
		}
	    }
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "options multi and recur cannot be used together") if($this->recur() and $this->multi());
	}
        elsif ($option->name() eq 'simWeight') {
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "option simWeight must have exactly one sub-option") unless(scalar(keys(%{$option->options()})) == 1);
	    $this->simWeight((keys(%{$option->options()}))[0]);
        }
	else {
	    Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option ".$option->name());
	}
    }

    my $count = 0;
    for my $t ($this->types()) {
	my $p = Mace::Compiler::Param->new(name => "p$count", type => $t);
	$this->push_params($p);
	$count++;
    }

    my $selector = $sv->selectors('message');
    $selector =~ s/\$function/schedule/;
    $selector =~ s/\$state/(timer)/;
    my $n = $this->name();
    $selector =~ s/\$message/$n/;
    $this->scheduleSelector($selector);
    $sv->selectorVars("schedule$n", $selector);
    $selector =~ s/schedule/cancel/;
    $this->cancelSelector($selector);
    $sv->selectorVars("cancel$n", $selector);
    $selector =~ s/cancel/expire/;
    $this->expireSelector($selector);
    $sv->selectorVars("expire$n", $selector);
}

sub toPrint {
  my $this = shift;
  my $out = shift;
  if($this->dump()) {
#    my $s = qq{<< "$name=" << (${name}.nextScheduled()?"scheduled":"unscheduled")};
    my $s = qq/$out << ${\$this->name};/;
    return $s;
  }
  return '';
}

sub toPrintNode {
    my $this = shift;
    my $printer = shift;
    my $name = $this->name();
    if ($this->dump()) {
	return qq/mace::printItem($printer, "$name", &${name});/;
    }
    return "";
}

sub toPrintState {
  my $this = shift;
  my $out = shift;
  if($this->dump()) {
#    my $s = qq{<< "$name=" << (${name}.nextScheduled()?"scheduled":"unscheduled")};
    my $s = qq/${\$this->name}.printState($out);/;
    return $s;
  }
  return '';
}

sub toSerialize {
  my $this = shift;
  my $str = shift;
  if($this->serialize()) {
    my $name = $this->name();
    my $s = qq/${\$this->name}.serialize($str);/;
    return $s;
  }
  return '';
}

sub toDeserialize {
    my $this = shift;
    my $in = shift;
    my %opt = @_;
    my $prefix = "";
    if($opt{prefix}) { $prefix = $opt{prefix}; }
    if ($this->serialize()) {
        my $name = $this->name();
        my $s = qq/$prefix ${\$this->name}.deserialize($in);/;
        return $s;
    }
    return '';
}

sub getLogLevel() {
  my $this = shift;
  my $def = shift;
  if ($this->trace() > -2) { return $this->trace(); }
  return $def;
}

sub isContext() {
  my $this = shift;
  if( defined $this->isContextTimer ) {
      return $this->isContextTimer;
  }
  return 0;
}

1;
