#ifndef __EMONITOR_h
#define __EMONITOR_h

#include "mace.h"
#include "Message.h"
#include "pthread.h"
#include "ThreadPool.h"
/**
 * \file ContextDispatch.h
 * \brief declares the ContextEventTP class
 */

namespace mace{

class ElasticityBehaviorEventHelper { };

class ElasticityColocateEvent: public ElasticityBehaviorEventHelper {
public:
  mace::vector<mace::string> colocateContexts;

public:
  ElasticityColocateEvent() { }
  ~ElasticityColocateEvent() { }
};

class ElasticityIsolateEvent: public ElasticityBehaviorEventHelper {
public:
  static const uint8_t RESOURCE_CPU = 1;

public:
  mace::string contextName;
  uint8_t resourceType;

public:
  ElasticityIsolateEvent( const mace::string& ctx_name, const uint8_t resource_type): contextName(ctx_name), resourceType(resource_type) { }
  ~ElasticityIsolateEvent() { }
};

class ElasticityBehaviorEvent {
public:
  static const uint8_t EBEVENT_UNDEF = 0;
  static const uint8_t EBEVENT_COLOCATE = 1;
  static const uint8_t EBEVENT_SEPARATE = 2;
  static const uint8_t EBEVENT_PIN = 3;
  static const uint8_t EBEVENT_ISOLATE = 4;

public:
  uint8_t eventType;
  ElasticityBehaviorEventHelper* ebhelper;

public:
  ElasticityBehaviorEvent( const uint8_t eventType, ElasticityBehaviorEventHelper* helper): eventType(eventType), ebhelper( helper ) { }
  ~ElasticityBehaviorEvent() { delete ebhelper; }

  void deleteHelper();
  mace::set<mace::string> getContexts();

};

class MigrationContextInfo;

class ElasticityBehaviorAction {
public:
  static const uint8_t EBACTION_UNDEF = 0;
  static const uint8_t EBACTION_MIGRATION = 1;
  static const uint8_t EBACTION_PIN = 3;

public:
  uint8_t actionType;
  uint8_t specialRequirement;
  mace::MaceAddr destNode;
  mace::string contextName;
  double benefit;
  double curLatency;
  double threshold;
  
public:
  ElasticityBehaviorAction( const uint8_t action_type, const mace::MaceAddr& dest_node, const mace::string& context_name, const double& benefit ): 
    actionType(action_type), specialRequirement(0), destNode(dest_node), contextName(context_name), benefit(benefit) { }
  ~ElasticityBehaviorAction() { }

  bool isConfictWith(const ElasticityBehaviorAction* action) const;
  mace::string getContextName() const { return contextName; }

  MigrationContextInfo generateMigrationContextInfo( const double& contextExecTime, const mace::map< mace::string, uint64_t>& contextsInterCount, 
    const mace::map< mace::string, uint64_t>& contextsInterSize, const uint64_t& count );

};

class eMonitor;
struct ElasticityThreadArg {
  eMonitor* elasticityMonitor;
};

class CPUInformation: public Serializable, public PrintPrintable {
public:
  uint64_t totalUserCPUTime;
  uint64_t totalUserLowCPUTime;
  uint64_t totalSysCPUTime;
  uint64_t totalIdleCPUTime;
  
public:
  CPUInformation(): totalUserCPUTime(0), totalUserLowCPUTime(0), totalSysCPUTime(0), totalIdleCPUTime(0) { }
  ~CPUInformation() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
  void print(std::ostream& out) const { }
  void printNode(PrintNode& pr, const std::string& name) const { }  

  CPUInformation& operator=(const CPUInformation& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->totalUserCPUTime = orig.totalUserCPUTime;
    this->totalUserLowCPUTime = orig.totalUserLowCPUTime;
    this->totalSysCPUTime = orig.totalSysCPUTime;
    this->totalIdleCPUTime = orig.totalIdleCPUTime;
    return *this;
  }
};

class MigrationRequest {
public:
  mace::string contextName;
  mace::MaceAddr destNode;

public:
  MigrationRequest() { }
  ~MigrationRequest() { }

  void runtest() { }

};

class ContextRuntimeInfo;
class ElasticityConfiguration;

class ServerRuntimeInfo: public Serializable, public PrintPrintable {
public:
  double CPUUsage;
  double totalCPUTime;
  double contextMigrationThreshold;
  uint32_t contextsNumber;
  uint64_t totalClientRequestNumber;

public:
  ServerRuntimeInfo(): CPUUsage(0.0), totalCPUTime(0.0), contextMigrationThreshold(0.0), contextsNumber(0), totalClientRequestNumber(0) { }
  ServerRuntimeInfo( const double& cpu_usage, const double& total_cpu_time, const double& ctx_migration_threshold, const uint32_t& n_ctx,
    const uint64_t& client_req_number ): CPUUsage(cpu_usage), totalCPUTime( total_cpu_time ), contextMigrationThreshold(ctx_migration_threshold), 
    contextsNumber(n_ctx), totalClientRequestNumber(client_req_number) { }
  ~ServerRuntimeInfo() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const;

  void printNode(PrintNode& pr, const std::string& name) const { } 

public:
  double adjustCPUUsageWhenToAddContext( const double& exec_time ) const { 
    return 100*(exec_time + totalCPUTime*CPUUsage*0.01) / totalCPUTime; 
  }
};


class ServersRuntimeInfo {
public:
  static const uint8_t NORMAL = 0;
  static const uint8_t SCALE_DOWN = 1;
  static const uint8_t NO_ACTION = 2;

private:
  mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > serversInfo;
  pthread_mutex_t runtimeInfoMutex;
  mace::set< mace::MaceAddr > servers;
  mace::set< mace::MaceAddr > activeServers;
  
public:
  ServersRuntimeInfo();
  ~ServersRuntimeInfo();

  bool updateServerInfo(const mace::MaceAddr& src, const ServerRuntimeInfo& server_info );
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo > getActiveServersInfo(bool isGEM) const;

  const mace::set<mace::MaceAddr>& getServerAddrs() const { return servers; }
  const mace::set<mace::MaceAddr>& getActiveServerAddrs() const { return activeServers; }
  void setServerAddrs( const mace::vector<mace::MaceAddr>& addrs, const uint32_t init_active_server_per );

  double getServerCPUUsage( const mace::MaceAddr& addr );
  double getServerCPUTime( const mace::MaceAddr& addr );
  uint64_t getServerClienRequestNumber( const mace::MaceAddr& addr );

  void scaleup( const uint64_t& n );
  mace::vector< mace::pair<MaceAddr, MaceAddr> > scaledown();
};

class MigrationContextInfo: public Serializable, public PrintPrintable {
public:
  static const uint8_t NONE = 0;
  static const uint8_t IDLE_CPU = 1;
  static const uint8_t MAX_LATENCY = 2;
  static const uint8_t BUSY_CPU = 3;
  static const uint8_t IDLE_NET = 4;

public:
  mace::string contextName;
  double currLatency;
  double contextExecTime;
  mace::map< mace::string, uint64_t> contextsInterCount;
  mace::map< mace::string, uint64_t> contextsInterSize;
  uint64_t count;
  uint8_t specialRequirement;
  double threshold;
  double benefit;

public:
  MigrationContextInfo(): contextName(""), currLatency(0), contextExecTime(0), specialRequirement(NONE) { }
  MigrationContextInfo( const mace::string& contextName, const double& currLatency, const double& contextExecTime, 
    const mace::map< mace::string, uint64_t>& contextsInterCount, const mace::map< mace::string, uint64_t>& contextsInterSize,
    const uint64_t& count ):
    contextName( contextName ), currLatency( currLatency ), contextExecTime( contextExecTime ), 
    contextsInterCount( contextsInterCount ), contextsInterSize( contextsInterSize ), count(count), specialRequirement(NONE) { }
  ~MigrationContextInfo() { }

  void updateBenefitForInteraction( const mace::string& ctx_name, const mace::ContextMapping& snapshot);

public:
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const;

  void printNode(PrintNode& pr, const std::string& name) const { }

public:
  uint64_t getTotalClientRequestNumber() const;
};

class ContextMigrationQuery {
public:
  mace::MaceAddr srcAddr;
  mace::vector<mace::MigrationContextInfo> migrationContextsInfo;

public:
  ContextMigrationQuery() { }
  ContextMigrationQuery( const mace::MaceAddr& src_Addr, const mace::vector<mace::MigrationContextInfo>& m_ctxs_info ): srcAddr( src_Addr ), 
    migrationContextsInfo( m_ctxs_info ) { }
  ~ContextMigrationQuery() { migrationContextsInfo.clear(); }

};

class MigratedContextInfo {
public:
  double latency;
  double benefitPercent;

public:
  MigratedContextInfo( ): latency(0.0), benefitPercent(0.0) { }
  MigratedContextInfo( const double& latency, const double& benefit_percent ): latency(latency), benefitPercent(benefit_percent) { }
  ~MigratedContextInfo() { }
};

class ContextRuntimeInfoForElasticity {
public:
  mace::string contextName;
  mace::map< mace::string, uint64_t > fromAccessCount;
  mace::map< mace::string, uint64_t > fromMessageSize;

  mace::map< mace::string, uint64_t > toAccessCount;

  double currLatency;
  double contextExecTime;
  double avgLatency;
  uint64_t count;

public:
  ContextRuntimeInfoForElasticity(): currLatency(0.0), contextExecTime(0.0), avgLatency(0.0), count(0)  { }
  ~ContextRuntimeInfoForElasticity() { }

public:
  mace::map< mace::string, uint64_t > getFromAccessCounts() { return fromAccessCount; }

  mace::map< mace::MaceAddr, uint64_t > computeExchangeBenefit( const mace::ContextMapping& snapshot );
  uint64_t getTotalFromAccessCount() const;

  bool isStrongConnected( const mace::string& ctx_name ) const;
  uint64_t getTotalClientRequestNumber() const;
  mace::set<mace::string> getStrongConnectContexts() const;
};

class eMonitor {
public:
  static const uint64_t EVENT_COACCESS_THRESHOLD = 10;
  static const uint64_t MIGRATION_CPU_THRESHOLD = 100;
  static const uint64_t CONTEXT_STRONG_CONNECTED_PER_THREAHOLD = 0.6;
  static const uint64_t CONTEXT_STRONG_CONNECTED_NUM_THREAHOLD = 6000;
  static const uint64_t INTER_CONTEXTS_STRONG_CONNECTED_THREAHOLD = 1;
  static const double CPU_BUSY_THREAHOLD = 70;
  static const double CPU_IDLE_THREAHOLD = 30;

  static const uint32_t MAJOR_HANDLER_THREAD_ID = 1;

  static const double DECREASE_MIGRATION_THRESHOLD_PERCENT = 0.7;
  static const double INCREASE_MIGRATION_THRESHOLD_PERCENT = 0.5;
  static const double MIGRATION_THRESHOLD_STEP = 1.0;

  static const double CLOSE_LATENCY_PERCENT = 0.7;

  static const double CONEXT_NUMBER_BALANCE_THRESHOLD = 0.1;
  static const int MIN_CONEXT_NUMBER_BALANCE = 2;

private:
  AsyncEventReceiver* sv;
  bool isGEM;
    
  pthread_t collectorKey;
  uint32_t periodTime;
  
  pthread_t ebHandlerKey;
  bool halted;
  ElasticityBehaviorAction* ebAction;
  
  mace::map< mace::string, mace::ContextRuntimeInfo> contextRuntimeInfos;

  CPUInformation lastcpuInfo;
  double currentcpuUsage;

  ServersRuntimeInfo serversRuntimeInfo;

  // Elasticity control
  double predictCPUTime;
  double predictCPUTotalTime;
  uint64_t predictTotalClientRequests;
  mace::set< mace::string > predictLocalContexts;
  uint64_t scaleOpType;
  mace::MaceAddr scaledownAddr;

  // Elasticity control - exchange contexts
  mace::map< mace::MaceAddr, mace::vector< mace::pair<mace::string, uint64_t> > > exchangeContexts;
  mace::map< mace::MaceAddr, uint64_t > exchangeBenefits;
  uint16_t exchangeProcessStep;
  mace::MaceAddr waitingReplyAddr;

  std::map< mace::string, ContextRuntimeInfoForElasticity > contextsRuntimeInfo;

  mace::set<mace::string> manageContextTypes;
  
  // migration
  pthread_mutex_t migrationMutex;
  std::map< mace::string, MigrationRequest > contextMigrationRequests;
  std::vector< mace::string > migratingContextNames;
  mace::string migratingContextName;
  bool readyToProcessMigrationQuery;
  bool migrationQueryThreadIsIdle;
  std::vector< mace::ContextMigrationQuery > contextMigrationQueries; 
  bool toMigrateContext;
  double contextMigrationThreshold;
  std::map<mace::string, MigratedContextInfo> migratedContextsInfo;

public:
  mace::ElasticityConfiguration* eConfig;

public:
  static void* startInfoCollectorThread( void* arg );
  static void* startElasticityBehaviorHandlerThread( void* arg );
  
public:
  eMonitor( AsyncEventReceiver* sv, const uint32_t& period_time, const mace::set<mace::string>& manange_contexts );
  ~eMonitor() { }

  void setIsGEM( bool is_gem ) { isGEM = is_gem; }

  double computeCurrentCPUUsage() const;
  uint64_t computeCPUTotalTime() const;

  double computeCurrentMemUsage() const;
  double getCurrentCPUUsage() const { return currentcpuUsage; }

  void handleElasticityScaleup( mace::MaceAddr const& idle_servers );
  mace::set<mace::string> generateMigrationContextSet();

  void runElasticityBehaviorHandle();
  bool hasPendingElasticityBehaviorActions();
  void executeElasticityBehaviorActionSetup();
  void executeElasticityBehaviorActionProcess();

  static void tryWakeupEbhandler();
  static void enqueueElasticityBehaviorAction( ElasticityBehaviorAction* ebAvent );

  static double predictBenefit( const double& ctx_cpu_usage, const double& server_cpu_usage1, const double& local_msg_count1, 
    const double& remote_msg_count1, const double& server_cpu_usage2, const double& local_msg_count2, 
    const double& remote_msg_count2 );

  static double estimateBenefitPercent( const double& threshold, const double& ctx_cpu_usage, 
    const double& server_cpu_usage1, const double& local_msg_count1, const double& remote_msg_size1, 
    const double& server_cpu_usage2, const double& local_msg_count2, const double& remote_msg_size2 );

  static void resortMigrationContextsInfo( mace::vector<MigrationContextInfo>& m_ctxes_info );



  void runInfoCollection();
  void collectContextsInfos(const uint64_t& round);
  void updateCPUInfo();

  void processContextElasticity(const mace::string& marker, const bool& migrate_flag);

  void updateServerInfo(const mace::MaceAddr& src, const mace::ServerRuntimeInfo& server_info );
  
  void setServerAddrs( const mace::vector<mace::MaceAddr>& addrs, const uint32_t init_active_server_per ) { serversRuntimeInfo.setServerAddrs(addrs, init_active_server_per); }
  
  // Elasticity control
  void processContextsMigrationQuery();
  void processContextsMigrationQueryReply( const mace::MaceAddr& dest, const mace::vector<mace::string>& accept_m_contexts );
  void enqueueContextMigrationQuery( const mace::MaceAddr& src, const mace::vector<mace::MigrationContextInfo>& m_contexts_info );
  void setScaleOpType( const uint64_t& type, const mace::MaceAddr& addr ) { scaleOpType = type; scaledownAddr = addr; }

  // migration
  void requestContextMigration(const mace::string& contextName, const MaceAddr& destNode);
  void processContextMigration();
  void wrapupCurrentContextMigration();

private:
  CPUInformation getCurrentCPUInfo() const;
  mace::vector<mace::string> sortContextsByCPU() const;

  std::vector<mace::ElasticityBehaviorAction*> generateActionsForElasticityBehavior( ElasticityBehaviorEvent* ebevent );
  bool isValidSequentialActions( const std::vector< mace::ElasticityBehaviorAction*>& seq_action);
  double computeSeqentialActionScore( const std::vector<ElasticityBehaviorAction*>& seq_action );
  double computeActionScore( const ElasticityBehaviorAction* action );
  double computeMigrationConnectionScore( const uint32_t& local_conn_strength, const uint32_t& remote_conn_strengh ) const;
  double computeMigrationCPUScore( const double& local_server_cpu_usage, const double& remote_server_cpu_usage, const double& ctx_cpu_usage) const;
  mace::ElasticityBehaviorAction* getNextAction( std::vector< mace::ElasticityBehaviorAction* >& actions );

  double adjustContextMigrationThreshold( std::vector<mace::ContextBaseClass*>& contexts, const mace::string& marker );

  void processContextElasticityPageRankOpt( const mace::string& marker, std::vector<mace::ContextBaseClass*>& contexts );
  void processContextElasticityPageRankIso( const mace::string& marker, std::vector<mace::ContextBaseClass*>& contexts );

  void processContextElasticityRules( const mace::string& marker, std::vector<mace::ContextBaseClass*>& contexts );
  void processContextElasticityByInteraction(std::vector<mace::ContextBaseClass*>& contexts);
  void processNextContextsExchangeRequest();
  void processExchangeContextsQuery();
  void processExchangeContextsQueryReply( const mace::MaceAddr& dest, const mace::vector<mace::string>& contexts );
};

}
#endif

