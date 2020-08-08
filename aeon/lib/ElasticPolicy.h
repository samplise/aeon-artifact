#ifndef _ELASTICPOLICY_H
#define _ELASTICPOLICY_H

// including headers
#include "mace.h"
#include "m_map.h"
#include "event.h"
#include "eMonitor.h"

class ContextStructure;

namespace mace{

class EventAccessInfo: public Serializable, public PrintPrintable {
private:
  mace::string eventType;
  bool isTargetContext;
  
  uint64_t totalFromEventCount;
  uint64_t totalToEventCount;

  uint64_t totalEventExecuteTime;
  mace::map< mace::string, uint64_t > fromContextCounts;
  mace::map< mace::string, uint64_t > fromMessagesSize;

  mace::map< mace::string, uint64_t > toContextCounts;

public:
  EventAccessInfo( mace::string const& eventType, bool isTargetContext): eventType(eventType), isTargetContext(isTargetContext), 
    totalFromEventCount(0), totalToEventCount(0), totalEventExecuteTime(0) { }
  EventAccessInfo(): totalFromEventCount(0), totalToEventCount(0), totalEventExecuteTime(0) { }
  ~EventAccessInfo() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { }  

  void addFromEventAccess(const uint64_t execute_time, const mace::string& create_ctx_name, const uint64_t& msg_size );
  void addToEventAccess(const mace::string& to_ctx_name ); 
  mace::map< mace::string, uint64_t > getFromEventAccessCount( const mace::string& context_type ) const;
  mace::map< mace::string, uint64_t > getFromEventAccessCount() const { return fromContextCounts; }
  mace::map< mace::string, uint64_t > getToEventAccessCount() const { return toContextCounts; }
  mace::map< mace::string, uint64_t > getFromEventMessageSize() const { return fromMessagesSize; }
  uint64_t getTotalExecuteTime() const { return totalEventExecuteTime; }
};

class ContetxtInteractionInfo: public Serializable, public PrintPrintable {
public:
  mace::map<mace::string, uint64_t> callerMethodCount;
  mace::map<mace::string, uint64_t> calleeMethodCount;

public:
  ContetxtInteractionInfo() { }
  ~ContetxtInteractionInfo() { 
    callerMethodCount.clear();
    calleeMethodCount.clear();
  }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { }  

  void addCallerContext( mace::string const& methodType, uint64_t count );
  void addCalleeContext( mace::string const& methodType, uint64_t count );

  uint64_t getInterCount() const { return (calleeMethodCount.size() + callerMethodCount.size()); }
};

class EventRuntimeInfo: public Serializable, public PrintPrintable {
public:
  mace::OrderID eventId;
  mace::string eventMethodType;
  uint64_t curEventExecuteTime;
  uint64_t curEventExecuteTimestamp;
  bool isTargetContext;
  mace::string createContextName;
  uint64_t messageSize;
  
public:
  EventRuntimeInfo( mace::OrderID const& eventId, mace::string const& eventMethodType): eventId(eventId), eventMethodType(eventMethodType),
    curEventExecuteTime(0), curEventExecuteTimestamp(0), isTargetContext(false), messageSize(0) { }
  EventRuntimeInfo(): eventId(), eventMethodType(""), curEventExecuteTime(0), curEventExecuteTimestamp(0), isTargetContext(false),
    messageSize(0) { }
  ~EventRuntimeInfo() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { }

  // void addCallerContext( mace::string const& callerContext, mace::string const& methodType);
  // void addCalleeContext( mace::string const& calleeContext, mace::string const& methodType);
};

class ContextRuntimeInfo: public Serializable, public PrintPrintable {
public:
  pthread_mutex_t runtimeInfoMutex;
  
  mace::string contextName;
  uint64_t curPeriodStartTimestamp;
  uint64_t curPeriodEndTimestamp;
  uint64_t timePeriod;

  mace::map<mace::string, uint64_t> markerStartTimestamp;
  mace::map<mace::string, uint64_t> markerTotalTimeperiod;
  mace::map<mace::string, uint64_t> markerTotalCount;
  
  // event access information
  mace::map<mace::string, EventAccessInfo> eventAccessInfos;
  
  // context interaction information
  mace::map<mace::string, ContetxtInteractionInfo> contextInterInfos;

  mace::map<mace::OrderID, EventRuntimeInfo> eventRuntimeInfos;

  mace::map< mace::string, uint64_t > coaccessContextsCount;

public:
  ContextRuntimeInfo(): curPeriodStartTimestamp(0), curPeriodEndTimestamp(0), timePeriod(0) { 
    pthread_mutex_init( &runtimeInfoMutex, NULL );
  }

  ~ContextRuntimeInfo() {
    this->clear();
    pthread_mutex_destroy( &runtimeInfoMutex );
  }

  mace::ContextRuntimeInfo& operator=( mace::ContextRuntimeInfo const& orig );

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { } 

  void clear();

  void setTimePeriod(const uint64_t timePeriod) { this->timePeriod = timePeriod; }
  void setContextName(const mace::string& contextName) { this->contextName = contextName; }


  void runEvent( const mace::Event& event );
  void addEventMessageSize( const mace::Event& event, const uint64_t msg_size );
  void stopEvent( const mace::OrderID& eventId );
  void addCalleeContext( mace::string const& calleeContext, mace::string const& methodType );
  void commitEvent( const mace::OrderID& eventId );
  void addCoaccessContext( mace::string const& context );

  void addToEventAccess( const mace::string& method_type, const mace::string& to_ctx_name );

  uint64_t getCPUTime();
  
  mace::string getCoaccessContext( mace::string const& context_type );

  uint32_t getConnectionStrength( const mace::ContextMapping& snapshot, const mace::MaceAddr& addr );
  mace::vector<mace::string> getInterctContextNames( mace::string const& context_type );
  mace::vector<mace::string> getEventInterctContextNames( mace::string const& context_type );
  void printRuntimeInformation( const uint64_t& total_ctx_execution_time, const double& server_cpu_usage, 
    const mace::ContextMapping& snapshot, const uint64_t& r);

  void markStartTimestamp( mace::string const& marker );
  void markEndTimestamp( mace::string const& marker );

  mace::map< mace::MaceAddr, uint64_t > getServerInteractionCount(mace::ContextMapping const& snapshot );
  
  mace::map< mace::string, uint64_t > getContextInteractionCount();
  mace::map< mace::string, uint64_t > getFromAccessCountByTypes( const mace::set<mace::string>& context_types );
  mace::map< mace::string, uint64_t > getToAccessCountByTypes( const mace::set<mace::string>& context_types );

  mace::map< mace::string, uint64_t > getEstimateContextsInteractionSize();
  uint64_t getMarkerTotalLatency( const mace::string& marker );
  double getMarkerAvgLatency( const mace::string& marker );
  uint64_t getMarkerCount( const mace::string& marker );
  uint64_t getTotalFromAccessCount();
};

class ElasticityCondition: public Serializable, public PrintPrintable {
public:
  static const uint8_t NONE = 0;

public:
  static const uint8_t CONTEXT_TYPE = 1;
  static const uint8_t REFERENCE = 2;
  static const uint8_t MARKER_LATENCY_VALUE = 3;
  static const uint8_t MARKER_LATENCY_PERCENT = 4;
  static const uint8_t SLA_MAX_VALUE = 5;
  static const uint8_t ACCESS_COUNT_PERCENT = 6;
  static const uint8_t SERVER_CPU_USAGE = 7;
  static const uint8_t EVENT_ACCESS_COUNT_PERCENT = 8;

public:
  static const uint8_t COMPARE_EQ = 1;
  static const uint8_t COMPARE_LT = 2;
  static const uint8_t COMPARE_ME = 3;
  static const uint8_t COMPARE_LE = 4;
  
public:
  uint8_t conditionType;
  uint8_t compareType;
  mace::set<mace::string> contextTypes;
  mace::string marker;
  double threshold;
    
public:
  ElasticityCondition(): conditionType(NONE), compareType(NONE), marker("") { }
  ElasticityCondition( const uint8_t type): conditionType(type), compareType(NONE), contextTypes(), marker("") { }
  ~ElasticityCondition() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { } 

public:
  mace::set<mace::string> satisfyCondition( const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, 
    const ServerRuntimeInfo& server_info, const mace::set<mace::string>& checkContexts, const ContextStructure& ownerships ) const;
  bool isSLAMaxValueCondition() const;
  double getThreshold() const { return threshold; }
  mace::vector<mace::string> getReferenceParentContextNames( const ContextStructure& ownerships, const mace::string& ctx_name ) const;
  mace::string getParentContextType(const mace::string& ctx_name) const;
};

class ElasticityAndConditions: public Serializable, public PrintPrintable {
public:
  mace::vector< mace::ElasticityCondition > conditions;
  
public:
  ElasticityAndConditions() { }
  ~ElasticityAndConditions() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { } 

public:
  mace::vector<mace::string> satisfyAndConditions( const mace::vector<mace::string>& contexts, 
    const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, const ServerRuntimeInfo& server_info,
    const ContextStructure& ownerships ) const;
  bool isSLAMaxValueCondition() const;
  double getSLAMaxValue() const { ASSERT(conditions.size()==1); return conditions[0].getThreshold(); }
  bool includeReferenceCondition() const;
  mace::ElasticityCondition getReferenceCondition() const;
  mace::vector<mace::string> getReferenceParentContextNames( const ContextStructure& ownerships, const mace::string& ctx_name ) const;
};


class ElasticityBehavior: public Serializable, public PrintPrintable {
public:
  static const uint8_t INVALID = 0;
  static const uint8_t COLOCATE = 4;
  static const uint8_t SEPARATE = 1;
  static const uint8_t PIN = 2;
  static const uint8_t ISOLATE = 3;
  static const uint8_t WORKLOAD_BALANCE = 5;
  static const uint8_t NUMBER_BALANCE = 6;

public:
  static const uint8_t RES_NONE = 0;
  static const uint8_t RES_CPU = 1;
  static const uint8_t RES_NET = 2;
  
public:
  uint8_t behaviorType;
  uint8_t resourceType;
  mace::vector< mace::string > contextTypes;
  mace::vector< mace::string > contextNames;


public:
  ElasticityBehavior(): behaviorType( INVALID), resourceType(RES_NONE) { }
  ~ElasticityBehavior() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { } 

public:
  
  
};

class ElasticityRule: public Serializable, public PrintPrintable {
public:
  static const uint8_t NO_RULE = 0;
  static const uint8_t ACTOR_RULE = 1;
  static const uint8_t GLOBAL_RULE = 2;
  static const uint8_t SLA_ACTOR_MARKER_RULE = 3;
  static const uint8_t INIT_PLACEMENT = 4;

public:
  uint8_t ruleType;
  
public:
  uint16_t priority;
  mace::vector< mace::ElasticityAndConditions > conditions;
  mace::set< mace::string > relatedContextTypes;
  mace::ElasticityBehavior behavior;

public:
  ElasticityRule(): ruleType(NO_RULE), priority(0) { }
  ElasticityRule( const uint8_t& type ): ruleType(type), priority(0) { }
  ~ElasticityRule() { }

  mace::set<mace::string> satisfyGlobalRuleConditions( const mace::vector<mace::string>& contexts, 
    const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, const ServerRuntimeInfo& server_info,
    const ContextStructure& ownerships  ) const; 
  const mace::ElasticityBehavior& getElasticityBehavior() const { return behavior; }
  
  bool isSLAMaxValueRule() const;
  double getSLAMaxValue() const { ASSERT(conditions.size()==1); return conditions[0].getSLAMaxValue(); }
  bool relatedContextType( const mace::string& context_type ) const;
  bool includeReferenceCondition() const;
  mace::ElasticityCondition getReferenceCondition() const;

  mace::ElasticityBehaviorAction* generateElasticityAction( const ContextRuntimeInfoForElasticity& ctx_runtime_info, 
    const mace::ContextMapping& snapshot, const ContextStructure& ownerships, 
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info,
    mace::map< mace::MaceAddr, double >& servers_cpu_usage, mace::map< mace::MaceAddr, double >& servers_cpu_time,
    mace::map< mace::MaceAddr, uint64_t >& servers_creq_count, 
    mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets ) const;

public:
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { }
};

class ElasticityConfiguration {
private:
  mace::vector< mace::ElasticityRule > elasticityRules;

public:
  ElasticityConfiguration() { }
  ~ElasticityConfiguration() { }

  void readElasticityPolicies();
  void addElasticityRule( ElasticityRule const& rule ) { elasticityRules.push_back(rule); }

  const mace::vector< mace::ElasticityRule >& getElasticityRules() const { return elasticityRules; }
  mace::vector< mace::ElasticityRule > getRulesByContextType( const mace::string& context_type ) const;
  mace::ElasticityRule getContextInitialPlacement( const mace::string& context_type ) const;
};

}

#endif
