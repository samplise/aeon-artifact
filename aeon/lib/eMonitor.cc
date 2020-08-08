#include "eMonitor.h"
#include "SysUtil.h"
#include "ContextService.h"

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "sys/types.h"
#include "sys/sysinfo.h"

/******************************* class ElasticityBehaviorEvent ***********************************************************/
void mace::ElasticityBehaviorEvent::deleteHelper() {
	if( ebhelper != NULL ) {
      delete ebhelper;
      ebhelper = NULL;
    }
}

mace::set<mace::string> mace::ElasticityBehaviorEvent::getContexts() {
	mace::set<mace::string> contexts;
	if( eventType == EBEVENT_COLOCATE ){
		ElasticityColocateEvent* e = static_cast<ElasticityColocateEvent*>(ebhelper);
		mace::vector<mace::string>& v = e->colocateContexts;

		for( uint32_t i=0; i<v.size(); i++ ){
			contexts.insert( v[i] );
		}
	} else if( eventType == EBEVENT_ISOLATE ) {
		ElasticityIsolateEvent* e = static_cast<ElasticityIsolateEvent*>(ebhelper);
		contexts.insert( e->contextName );
	}

	return contexts;
}

/******************************* class ElasticityBehaviorAction *************************************************/
bool mace::ElasticityBehaviorAction::isConfictWith(const ElasticityBehaviorAction* action) const {
	return false;
}

mace::MigrationContextInfo mace::ElasticityBehaviorAction::generateMigrationContextInfo( const double& contextExecTime, 
		const mace::map< mace::string, uint64_t>& contextsInterCount, const mace::map< mace::string, uint64_t>& contextsInterSize, 
		const uint64_t& count ) {
	ADD_SELECTORS("mace::ElasticityBehaviorAction");

	mace::MigrationContextInfo m_context_info( this->contextName, this->curLatency, contextExecTime, contextsInterCount,
		contextsInterSize, count);

	if( this->specialRequirement == mace::MigrationContextInfo::MAX_LATENCY ){
		m_context_info.specialRequirement = this->specialRequirement;
		m_context_info.threshold = this->threshold;
		m_context_info.benefit = this->benefit;
	}

	return m_context_info;
}

/******************************* class CPUInformation ***********************************************************/
void mace::CPUInformation::serialize(std::string& str) const{
  mace::serialize( str, &totalUserCPUTime );
  mace::serialize( str, &totalUserLowCPUTime );
  mace::serialize( str, &totalSysCPUTime );
  mace::serialize( str, &totalIdleCPUTime );
}

int mace::CPUInformation::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &totalUserCPUTime );
  serializedByteSize += mace::deserialize( is, &totalUserLowCPUTime );
  serializedByteSize += mace::deserialize( is, &totalSysCPUTime   );
  serializedByteSize += mace::deserialize( is, &totalIdleCPUTime   );
  return serializedByteSize;
}

/******************************* class ServerRuntimeInfo **************************************************/
void mace::ServerRuntimeInfo::serialize(std::string& str) const{
  mace::serialize( str, &CPUUsage );
  mace::serialize( str, &totalCPUTime );
  mace::serialize( str, &contextMigrationThreshold );
  mace::serialize( str, &contextsNumber );
  mace::serialize( str, &totalClientRequestNumber );
}

int mace::ServerRuntimeInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &CPUUsage );
  serializedByteSize += mace::deserialize( is, &totalCPUTime );
  serializedByteSize += mace::deserialize( is, &contextMigrationThreshold );
  serializedByteSize += mace::deserialize( is, &contextsNumber );
  serializedByteSize += mace::deserialize( is, &totalClientRequestNumber );
  return serializedByteSize;
}

void mace::ServerRuntimeInfo::print(std::ostream& out) const {
  out<< "ServerRuntimeInfo(";
  out<< "CPUUsage="; mace::printItem(out, &(CPUUsage) ); out<<", ";
  out<< "totalCPUTime="; mace::printItem(out, &(totalCPUTime) ); out<<", ";
  out<< "contextMigrationThreshold="; mace::printItem(out, &(contextMigrationThreshold) ); out<<", ";
  out<< "contextsNumber="; mace::printItem(out, &(contextsNumber) ); out<<", ";
  out<< "totalClientRequestNumber="; mace::printItem(out, &(totalClientRequestNumber) );
  out<< ")";
}

/******************************* class ServersRuntimeInfo **************************************************/
mace::ServersRuntimeInfo::ServersRuntimeInfo() {
	pthread_mutex_init( &runtimeInfoMutex, NULL );
}

mace::ServersRuntimeInfo::~ServersRuntimeInfo() {
	pthread_mutex_destroy( &runtimeInfoMutex);
}

bool mace::ServersRuntimeInfo::updateServerInfo(const mace::MaceAddr& src, const ServerRuntimeInfo& server_info ) {
	ADD_SELECTORS("ServersRuntimeInfo::updateServerInfo");
	ScopedLock sl( runtimeInfoMutex );
	if( serversInfo.find(src) != serversInfo.end() ) {
		serversInfo.clear();
	}
	serversInfo[ src ] = server_info;

	// macedbg(1) << "serversInfo.size=" << serversInfo.size() << ", servers.size=" << servers.size() << Log::endl;
	if( serversInfo.size() == servers.size() ) {
		double threshold = 0.0;
		for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::iterator iter = serversInfo.begin(); iter != serversInfo.end();
				iter ++ ) {
			if( iter == serversInfo.begin() ) {
				threshold = (iter->second).contextMigrationThreshold;
			} else if( threshold < (iter->second).contextMigrationThreshold ) {
				threshold = (iter->second).contextMigrationThreshold;
			}
		}

		for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::iterator iter = serversInfo.begin(); iter != serversInfo.end();
				iter ++ ) {
			(iter->second).contextMigrationThreshold = threshold;
		}
		return true;
	} else {
		return false;
	}
}

mace::map<mace::MaceAddr, mace::ServerRuntimeInfo > mace::ServersRuntimeInfo::getActiveServersInfo( bool isGEM ) const {
	if( !isGEM ){
		return serversInfo;
	}

	mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > servers_info;
	for( mace::map<mace::MaceAddr, mace::ServerRuntimeInfo >::const_iterator iter = serversInfo.begin(); iter != serversInfo.end();
			iter ++ ) {
		if( activeServers.count(iter->first) > 0 ){
			servers_info[ iter->first ] = iter->second;
		}
	}
	return servers_info;
}

void mace::ServersRuntimeInfo::scaleup( const uint64_t& n ) {
	ADD_SELECTORS("ServersRuntimeInfo::scaleup");
	uint64_t m = n;
	while( servers.size() > activeServers.size() && m > 0 ){
		mace::MaceAddr addr;
		for(mace::set<mace::MaceAddr>::iterator iter = servers.begin(); iter != servers.end(); iter ++) {
			if( activeServers.count(*iter) == 0 ) {
				activeServers.insert(*iter);
				macedbg(1) << "To active server: " << *iter << Log::endl;
				m --;
				break;
			}
		}
	}

}

mace::vector< mace::pair<MaceAddr, MaceAddr> > mace::ServersRuntimeInfo::scaledown() {
	ADD_SELECTORS("ServersRuntimeInfo::scaledown");
	mace::vector<mace::MaceAddr> sorted_servers;
	mace::map<mace::MaceAddr, double> servers_cpu;

	for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::iterator iter = serversInfo.begin(); iter != serversInfo.end(); iter ++ ){
		if( activeServers.count(iter->first) == 0 ) {
			continue;
		}

		servers_cpu[ iter->first ] = (iter->second).CPUUsage;
		bool insert_flag = false;
		for( mace::vector<mace::MaceAddr>::iterator viter = sorted_servers.begin(); viter != sorted_servers.end(); viter ++ ){
			if( (iter->second).CPUUsage < serversInfo[*viter].CPUUsage ) {
				sorted_servers.insert(viter, iter->first);
				insert_flag = true;
				break;
			}
		}

		if( !insert_flag ){
			sorted_servers.push_back( iter->first );
		}
	}

	mace::vector< mace::pair<MaceAddr, MaceAddr> > sd_servers;
	
	int hi = 0;
	int ti = sorted_servers.size() - 1;
	while( hi < ti ) {
		const mace::MaceAddr& haddr = sorted_servers[hi];
		const mace::MaceAddr& taddr = sorted_servers[ti];

		if( servers_cpu[taddr] + servers_cpu[haddr] > eMonitor::CPU_BUSY_THREAHOLD ) {
			ti --;
		} else {
			mace::pair<MaceAddr, MaceAddr> sd_server( haddr, taddr );
			macedbg(1) << "To scale down: " << haddr << " -> " << taddr << Log::endl;
			sd_servers.push_back( sd_server );
			activeServers.erase( haddr );
			servers_cpu[ taddr ] += servers_cpu[ haddr ];
			hi ++;
		}
	}

	return sd_servers;
}

void mace::ServersRuntimeInfo::setServerAddrs( const mace::vector<mace::MaceAddr>& addrs, const uint32_t init_active_server_per ) {
	ADD_SELECTORS("ServersRuntimeInfo::setServerAddrs");
	macedbg(1) << "Servers: " << addrs << Log::endl;

	uint32_t active_n_servers = (uint32_t)(init_active_server_per * addrs.size() / 100);
	if( active_n_servers == 0 ){
		active_n_servers = 1;
	}
	for( uint32_t i=0; i<addrs.size(); i++ ){
		servers.insert( addrs[i] );
		if( i<active_n_servers) {
			activeServers.insert( addrs[i] );
		}
	}
}

double mace::ServersRuntimeInfo::getServerCPUUsage( const mace::MaceAddr& addr ) {
	ScopedLock sl( runtimeInfoMutex );
	if( serversInfo.find(addr) != serversInfo.end() ) {
		return serversInfo[addr].CPUUsage;
	} else {
		return 0.0;
	}
}

double mace::ServersRuntimeInfo::getServerCPUTime( const mace::MaceAddr& addr ) {
	ScopedLock sl( runtimeInfoMutex );
	if( serversInfo.find(addr) != serversInfo.end() ) {
		return serversInfo[addr].totalCPUTime;
	} else {
		return 0.0;
	}
}

uint64_t mace::ServersRuntimeInfo::getServerClienRequestNumber( const mace::MaceAddr& addr ) {
	ScopedLock sl( runtimeInfoMutex );
	if( serversInfo.find(addr) != serversInfo.end() ) {
		return serversInfo[addr].totalClientRequestNumber;
	} else {
		return 0;
	}
}

/******************************* class MigrationContextInfo **************************************************/
void mace::MigrationContextInfo::updateBenefitForInteraction( const mace::string& ctx_name, const mace::ContextMapping& snapshot ) {
	uint64_t inter_count = 0;
	if( contextsInterCount.find(ctx_name) != contextsInterCount.end() ) {
		inter_count = contextsInterCount[ctx_name];
		if( mace::ContextMapping::getNodeByContext( snapshot, contextName) == mace::ContextMapping::getNodeByContext(snapshot, ctx_name) ) {
			benefit += 2*inter_count;
		} else {
			benefit -= 2*inter_count;
		}
	}
}

uint64_t mace::MigrationContextInfo::getTotalClientRequestNumber() const {
	uint64_t count = 0;
	for( mace::map<mace::string, uint64_t>::const_iterator iter = contextsInterSize.begin(); iter != contextsInterSize.end(); iter ++ ){
		mace::string ctype = Util::extractContextType(iter->first);
		if( ctype == "externalCommContext" ) {
			count += iter->second;
		}
	}
	return count;
}


void mace::MigrationContextInfo::serialize(std::string& str) const{
  mace::serialize( str, &contextName );
  mace::serialize( str, &currLatency );
  mace::serialize( str, &contextExecTime );
  mace::serialize( str, &contextsInterCount );
  mace::serialize( str, &contextsInterSize );
  mace::serialize( str, &count );
  mace::serialize( str, &specialRequirement );
  mace::serialize( str, &threshold );
  mace::serialize( str, &benefit );
}

int mace::MigrationContextInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &contextName );
  serializedByteSize += mace::deserialize( is, &currLatency );
  serializedByteSize += mace::deserialize( is, &contextExecTime );
  serializedByteSize += mace::deserialize( is, &contextsInterCount );
  serializedByteSize += mace::deserialize( is, &contextsInterSize );
  serializedByteSize += mace::deserialize( is, &count );
  serializedByteSize += mace::deserialize( is, &specialRequirement );
  serializedByteSize += mace::deserialize( is, &threshold );
  serializedByteSize += mace::deserialize( is, &benefit );
  return serializedByteSize;
}

void mace::MigrationContextInfo::print(std::ostream& out) const {
  out<< "MigrationContextInfo(";
  out<< "contextName="; mace::printItem(out, &(contextName) ); out<<", ";
  out<< "currLatency="; mace::printItem(out, &(currLatency) ); out<<", ";
  out<< "contextExecTime="; mace::printItem(out, &(contextExecTime) ); out<<", ";
  out<< "contextsInterCount="; mace::printItem(out, &(contextsInterCount) );
  out<< ")";
}

/******************************* class ContextRuntimeInfoForElasticity ***********************************************************/
mace::map< mace::MaceAddr, uint64_t > mace::ContextRuntimeInfoForElasticity::computeExchangeBenefit( const mace::ContextMapping& snapshot ) {
  ADD_SELECTORS("ContextRuntimeInfoForElasticity::computeExchangeBenefit");
  mace::map< mace::MaceAddr, uint64_t > servers_comm_count;
  for( mace::map< mace::string, uint64_t >::iterator iter = fromAccessCount.begin(); iter != fromAccessCount.end(); iter ++ ){
  	mace::MaceAddr addr = mace::ContextMapping::getNodeByContext( snapshot, iter->first );
  	if( servers_comm_count.find(addr) == servers_comm_count.end() ) {
  		servers_comm_count[addr] = iter->second;
  	} else {
  		servers_comm_count[addr] += iter->second;
  	}
  }

  mace::map< mace::MaceAddr, uint64_t > exchange_benefits;
  uint64_t local_comm_count = 0;
  if( servers_comm_count.find(Util::getMaceAddr()) != servers_comm_count.end() ) {
    local_comm_count = servers_comm_count[ Util::getMaceAddr()];
  }
  macedbg(1) << "Local msg count: " << local_comm_count << Log::endl;

  for( mace::map< mace::MaceAddr, uint64_t >::iterator iter = servers_comm_count.begin(); iter != servers_comm_count.end(); iter ++ ){
    if( iter->first == Util::getMaceAddr() ) {
      continue;
    }

    macedbg(1) << "Server("<< iter->first <<") msg count: " << iter->second << Log::endl;
    if( iter->second > local_comm_count ) {
      exchange_benefits[ iter->first ] = iter->second - local_comm_count;
    }
  }
  
  return exchange_benefits;
}

uint64_t mace::ContextRuntimeInfoForElasticity::getTotalFromAccessCount() const {
	uint64_t total_count = 0;
	for( mace::map< mace::string, uint64_t>::const_iterator iter = fromAccessCount.begin(); iter != fromAccessCount.end(); iter ++ ){
		total_count += iter->second;
	}
	return total_count;
} 

bool mace::ContextRuntimeInfoForElasticity::isStrongConnected( const mace::string& ctx_name ) const {
	mace::map<mace::string, uint64_t> total_counts;
	mace::map< mace::string, uint64_t>::const_iterator iter;

	for(iter = fromAccessCount.begin(); iter != fromAccessCount.end(); iter ++ ){
		mace::string context_type = Util::extractContextType(iter->first);
		if( context_type == "globalContext" || context_type == "externalCommContext") {
			continue;
		}
		if( total_counts.find(iter->first) == total_counts.end() ) {
			total_counts[iter->first] = iter->second;
		} else {
			total_counts[iter->first] += iter->second;
		}
	}
	 
	for(iter = toAccessCount.begin(); iter != toAccessCount.end(); iter ++ ){
		mace::string context_type = Util::extractContextType(iter->first);
		if( context_type == "globalContext" || context_type == "externalCommContext") {
			continue;
		}
		if( total_counts.find(iter->first) == total_counts.end() ) {
			total_counts[iter->first] = iter->second;
		} else {
			total_counts[iter->first] += iter->second;
		}
	}

	if( total_counts.find(ctx_name) == total_counts.end() ) {
		return false;
	}

	uint64_t total_count = total_counts[ctx_name];
	double count = 0;
	for( iter = total_counts.begin(); iter != total_counts.end(); iter ++ ){

		if( total_count >= iter->second ){
			count ++;
		}
	}

	double frequency = count / total_counts.size();
	if( frequency > eMonitor::CONTEXT_STRONG_CONNECTED_PER_THREAHOLD ) {
		return true;
	} else if( contextExecTime/count > eMonitor::CONTEXT_STRONG_CONNECTED_NUM_THREAHOLD ) {
		return true;
	} else {
		return false;
	}
}

mace::set<mace::string> mace::ContextRuntimeInfoForElasticity::getStrongConnectContexts() const {
	ADD_SELECTORS("ContextRuntimeInfoForElasticity::getStrongConnectContexts");
	mace::map< mace::string, uint64_t > total_counts;
	mace::map< mace::string, uint64_t >::const_iterator iter;

	uint64_t total_count = 0;

	for(iter = fromAccessCount.begin(); iter != fromAccessCount.end(); iter ++ ){
		mace::string context_type = Util::extractContextType(iter->first);
		if( context_type == "globalContext" || context_type == "externalCommContext") {
			continue;
		}
		if( total_counts.find(iter->first) == total_counts.end() ) {
			total_counts[iter->first] = iter->second;
		} else {
			total_counts[iter->first] += iter->second;
		}
		total_count += iter->second;
	}
	 
	for(iter = toAccessCount.begin(); iter != toAccessCount.end(); iter ++ ){
		mace::string context_type = Util::extractContextType(iter->first);
		if( context_type == "globalContext" || context_type == "externalCommContext") {
			continue;
		}
		if( total_counts.find(iter->first) == total_counts.end() ) {
			total_counts[iter->first] = iter->second;
		} else {
			total_counts[iter->first] += iter->second;
		}
		total_count += iter->second;
	}
	double avg_count = 0.0;
	if( total_counts.size() > 0 ){
		avg_count = total_count / total_counts.size();
	}

	mace::set<mace::string> strong_conn_contexts;

	for( mace::map<mace::string, uint64_t>::iterator iter1 = total_counts.begin(); iter1 != total_counts.end(); iter1 ++ ){
		macedbg(1) << "context("<< iter1->first <<") count: " << iter1->second << Log::endl;
		// double count = 0.0;
		// for( mace::map<mace::string, uint64_t>::iterator iter2 = total_counts.begin(); iter2 != total_counts.end(); iter2 ++ ) {
		// 	if( iter1->second >= iter2->second ){
		// 		count ++;
		// 	}
		// }
		// double p = count / total_counts.size();
		if( // p > eMonitor::CONTEXT_STRONG_CONNECTED_PER_THREAHOLD || 
				iter1->second > eMonitor::CONTEXT_STRONG_CONNECTED_NUM_THREAHOLD
				|| iter1->second > avg_count ) {
			strong_conn_contexts.insert(iter1->first);
		}
	}
	return strong_conn_contexts;	
}

uint64_t mace::ContextRuntimeInfoForElasticity::getTotalClientRequestNumber() const {
	uint64_t count = 0;
	for( mace::map<mace::string, uint64_t>::const_iterator iter = fromAccessCount.begin(); iter != fromAccessCount.end(); iter ++ ){
		mace::string ctype = Util::extractContextType(iter->first);
		if( ctype == "externalCommContext" ){
			count += iter->second;
		}
	}
	return count;
}

/******************************* class eMonitor ***********************************************************/
pthread_mutex_t elasticityBehaviorActionQueueMutex = PTHREAD_MUTEX_INITIALIZER;
std::queue< mace::ElasticityBehaviorAction* > elasticityBehaviorActionQueue;
pthread_cond_t elasticityBehaviorSignal = PTHREAD_COND_INITIALIZER;

const uint32_t mace::eMonitor::MAJOR_HANDLER_THREAD_ID;
const double mace::eMonitor::MIGRATION_THRESHOLD_STEP;

mace::eMonitor::eMonitor( AsyncEventReceiver* sv, const uint32_t& period_time, const mace::set<mace::string>& manange_contexts ): 
		sv(sv), isGEM(false), periodTime(period_time), manageContextTypes(manange_contexts), migratingContextName(""), 
		migrationQueryThreadIsIdle(true), toMigrateContext(true), contextMigrationThreshold(0.5) {
	
	ADD_SELECTORS("eMonitor::eMonitor");

	// struct ElasticityThreadArg arg;
	// arg.elasticityMonitor = this;

	macedbg(1) << "periodTime=" << periodTime << ", sv=" << sv << ", eMonitor=" << this << Log::endl;

	eConfig = new ElasticityConfiguration();
	pthread_mutex_init( &migrationMutex, NULL );

	if( period_time > 0 ) {
		ASSERT(  pthread_create( &collectorKey , NULL, eMonitor::startInfoCollectorThread, static_cast<void*>(this) ) == 0 );
	}
	// ASSERT(  pthread_create( &ebHandlerKey , NULL, eMonitor::startElasticityBehaviorHandlerThread, (void*)&arg ) == 0 );
}

void mace::eMonitor::runElasticityBehaviorHandle() {
	ADD_SELECTORS("eMonitor::runElasticityBehaviorHandle");
	ScopedLock sl(elasticityBehaviorActionQueueMutex);
	while( !halted ){
		if( !hasPendingElasticityBehaviorActions() ){
			ASSERT(pthread_cond_wait(&elasticityBehaviorSignal, &elasticityBehaviorActionQueueMutex) == 0);
			continue;
      	}

		executeElasticityBehaviorActionSetup();
      	sl.unlock();
		executeElasticityBehaviorActionProcess();
		sl.lock();
	} 
	sl.unlock();
	pthread_exit(NULL);
}

bool mace::eMonitor::hasPendingElasticityBehaviorActions() {
	ADD_SELECTORS("eMonitor::hasPendingElasticityBehaviorActions");
    if( elasticityBehaviorActionQueue.empty() ){
		return false;
	} else {
		macedbg(1) << "There is a new ElasticityBehaviorAction!" << Log::endl;
		return true;
	}
}

void mace::eMonitor::executeElasticityBehaviorActionSetup() {
    ebAction = elasticityBehaviorActionQueue.front();
    elasticityBehaviorActionQueue.pop();
}

void mace::eMonitor::executeElasticityBehaviorActionProcess() {
	ADD_SELECTORS("eMonitor::executeElasticityBehaviorActionProcess");
	if( ebAction->actionType == mace::ElasticityBehaviorAction::EBACTION_MIGRATION  ) {
		this->requestContextMigration( ebAction->getContextName(), ebAction->destNode );
	}
    delete ebAction;
}

void mace::eMonitor::tryWakeupEbhandler(){
	pthread_cond_signal(&elasticityBehaviorSignal);
}

void mace::eMonitor::enqueueElasticityBehaviorAction( ElasticityBehaviorAction* ebAction ) {
	ADD_SELECTORS("eMonitor::enqueueElasticityBehaviorAction");
	macedbg(1) << "Enqueue an ElasticityBehaviorAction with type: " << (uint16_t)(ebAction->actionType) << Log::endl; 
	ScopedLock sl( elasticityBehaviorActionQueueMutex );
	elasticityBehaviorActionQueue.push( ebAction );
	sl.unlock();
	tryWakeupEbhandler();
}

double mace::eMonitor::predictBenefit( const double& ctx_cpu_usage, const double& server_cpu_usage1, const double& local_msg_count1, 
		const double& remote_msg_count1, const double& server_cpu_usage2, const double& local_msg_count2, 
		const double& remote_msg_count2 ) {
	ADD_SELECTORS("eMonitor::predictBenefit");

	macedbg(1) 	<< "ctx_cpu_usage=" << ctx_cpu_usage 
				<< ", server_cpu_usage1=" << server_cpu_usage1 
				<< ", server_cpu_usage2=" << server_cpu_usage2 
				<< ", local_msg_count1=" << local_msg_count1
				<< ", local_msg_count2=" << local_msg_count2
				<< ", remote_msg_count1=" << remote_msg_count1
				<< ", remote_msg_count2=" << remote_msg_count2
				<< Log::endl;

	double server_cpu_usage_weight1	= -0.00085476;
	double server_cpu_usage_weight2	= -0.00259529;
	double ctx_cpu_usage_weight1	= 0.00247086;
	double local_msg_count_weight1	= 0.04174204;
	double local_msg_count_weight2	= -0.03604542;
	double remote_msg_count_weight1	= -0.012535;
	double remote_msg_count_weight2 = 0.006525;

	double benefit1 =	server_cpu_usage_weight1	* server_cpu_usage1 +
						server_cpu_usage_weight2	* server_cpu_usage2 +	
						ctx_cpu_usage_weight1 		* ctx_cpu_usage + 
						local_msg_count_weight2 	* local_msg_count1 + 
						local_msg_count_weight2 	* local_msg_count2 +
						remote_msg_count_weight1	* remote_msg_count1 +
						remote_msg_count_weight2	* remote_msg_count2;

	double benefit2 =	server_cpu_usage_weight1	* server_cpu_usage2 +
						server_cpu_usage_weight2	* server_cpu_usage1 +	
						ctx_cpu_usage_weight1 		* ctx_cpu_usage + 
						local_msg_count_weight1 	* local_msg_count2 + 
						local_msg_count_weight2 	* local_msg_count1 +
						remote_msg_count_weight1 	* remote_msg_count2 +
						remote_msg_count_weight2 	* remote_msg_count1;

	macedbg(1) << "benefit1=" << benefit1 << ", benefit2=" << benefit2 << Log::endl;
	double benefit = (benefit1 - benefit2)/2;
	return benefit;
}

double mace::eMonitor::estimateBenefitPercent( const double& threshold, const double& ctx_cpu_usage, 
    	const double& server_cpu_usage1, const double& local_msg_count1, const double& remote_msg_size1, 
    	const double& server_cpu_usage2, const double& local_msg_count2, const double& remote_msg_size2 ) {
	ADD_SELECTORS("eMonitor::estimateBenefitPercent");

	macedbg(1) 	<< "threshold=" << threshold
				<< ", ctx_cpu_usage=" << ctx_cpu_usage 
				<< ", server_cpu_usage1=" << server_cpu_usage1 
				<< ", server_cpu_usage2=" << server_cpu_usage2 
				<< ", local_msg_count1=" << local_msg_count1
				<< ", local_msg_count2=" << local_msg_count2
				<< ", remote_msg_size1=" << remote_msg_size1
				<< ", remote_msg_size2=" << remote_msg_size2
				<< Log::endl;

	double x[8] = { server_cpu_usage1, server_cpu_usage2, ctx_cpu_usage, remote_msg_size1, remote_msg_size2, local_msg_count1, local_msg_count2, 1.0 };

	double lambda = 0.370118;
	
	double w[8] = { -0.00631546, -0.0174227, -0.0111499, 4.7407e-07, -4.1444e-07, 0.448032, -0.335277, 0.0738961 };

	double M[8][8] = { 
						{ 8.79735e-07, -5.33413e-09, -3.44326e-07, -2.62845e-13, -4.71782e-13, -4.98835e-06, -2.6767e-08, -5.929e-06 }, 
						{ -5.33413e-09, 7.17944e-07, -7.17426e-08, -1.69283e-14, -4.94263e-13, 2.9333e-08, -3.7604e-06, -9.28134e-06 }, 
						{ -3.44326e-07, -7.17426e-08, 5.24333e-06, -3.77483e-12, -4.04218e-13, 1.26278e-06, 4.0532e-07, -7.63106e-06 }, 
						{ -2.62845e-13, -1.69283e-14, -3.77483e-12, 2.90477e-16, -2.62051e-16, 3.7602e-11, -3.08075e-11, -9.55547e-11 }, 
						{ -4.71782e-13, -4.94263e-13, -4.04218e-13, -2.62051e-16, 2.94481e-16, -2.97821e-11, 3.77192e-11, -9.88994e-11 }, 
						{ -4.98835e-06, 2.9333e-08, 1.26278e-06, 3.7602e-11, -2.97821e-11, 3.575e-05, -3.75118e-06, -2.8504e-06 }, 
						{ -2.6767e-08, -3.7604e-06, 4.0532e-07, -3.08075e-11, 3.77192e-11, -3.75118e-06, 2.64242e-05, 1.06327e-05 }, 
						{ -5.929e-06, -9.28134e-06, -7.63106e-06, -9.55547e-11, -9.88994e-11, -2.8504e-06, 1.06327e-05, 0.00148201 }
					};

	uint32_t N = 8;
	double u = 0.0;
	for( uint32_t i=0; i<N; i++ ){
		u += x[i] * w[i];
	}

	double v = 0.0;
	double M1[8];
	for( uint32_t i=0; i<N; i++ ) {
		M1[i] = 0.0;
		for( uint32_t j=0; j<N; j++ ){
			M1[i] += x[j] * M[i][j];
		}
	}

	for( uint32_t i=0; i<N; i++ ) {
		v += M1[i] * x[i];
	}

	v += 1/lambda;

	// double arg = (threshold - u) / std::sqrt(v);

	// double p = 1 - erfc(-arg/std::sqrt(2))/2;

	double p = v;

	return p;
}

void mace::eMonitor::resortMigrationContextsInfo( mace::vector<MigrationContextInfo>& m_ctxes_info ) {
	while(true) {
		bool exchange_flag = false;
		for( uint32_t i=0; i+1<m_ctxes_info.size(); i++ ) {
			if( m_ctxes_info[i].benefit < m_ctxes_info[i+1].benefit ) {
				mace::MigrationContextInfo m_ctx_info = m_ctxes_info[i];
				m_ctxes_info[i] = m_ctxes_info[i+1];
				m_ctxes_info[i+1] = m_ctx_info;
				exchange_flag = true;
			}
		}
		if( !exchange_flag ) {
			break;
		}
	}
}

void* mace::eMonitor::startInfoCollectorThread(void* arg) {
	ADD_SELECTORS("eMonitor::startInfoCollectorThread");
	// struct ElasticityThreadArg* carg = ((struct ElasticityThreadArg*)arg);
	// macedbg(1) << "arg=" << arg << ", carg=" << carg << Log::endl;
	// eMonitor* monitor = carg->elasticityMonitor;
	eMonitor* monitor = static_cast<eMonitor*>(arg);
	macedbg(1) << "eMonitor=" << monitor << Log::endl;
	monitor->runInfoCollection();
	return 0;
}

void mace::eMonitor::runInfoCollection() {
	ADD_SELECTORS("eMonitor::runInfoCollection");
	macedbg(1) << "Start information collection thread with time period("<< periodTime <<"), sv=" << sv <<", eMonitor=" << this << Log::endl;
	while( true ){
		lastcpuInfo = this->getCurrentCPUInfo();
		SysUtil::sleep(periodTime, 0);
		// this->collectContextsInfos(0);
		this->processContextElasticity("", true);
	}
}

void mace::eMonitor::updateCPUInfo() {
	lastcpuInfo = this->getCurrentCPUInfo();
}

void mace::eMonitor::processContextElasticity( const mace::string& marker, const bool& migrate_flag ) {
	ADD_SELECTORS("eMonitor::processContextElasticity");

	macedbg(1) << "Start to process contexts elasticity!" << Log::endl;

	currentcpuUsage = this->computeCurrentCPUUsage();
	double total_cpu_time = this->computeCPUTotalTime() * 0.01;

	ContextService* _service = static_cast<ContextService*>(sv);
	std::vector<mace::ContextBaseClass*> contexts;
	_service->getContextObjectsByTypes(contexts, manageContextTypes);

	const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	// double adjust_ctx_m_threshold = this->adjustContextMigrationThreshold( contexts, marker );

	ScopedLock sl(migrationMutex);
	if( mace::ContextMapping::getHead(snapshot) == Util::getMaceAddr() ) {
		isGEM = true;
	} else {
		isGEM = false;
	}


	predictCPUTotalTime = total_cpu_time;
	predictCPUTime = predictCPUTotalTime * currentcpuUsage * 0.01;
	predictLocalContexts.clear();
	readyToProcessMigrationQuery = false;
	migrationQueryThreadIsIdle = true;
	toMigrateContext = migrate_flag;
	exchangeProcessStep = 0;
	contextMigrationQueries.clear();
	contextsRuntimeInfo.clear();
	
	predictTotalClientRequests = 0;
	for( uint32_t i=0; i<contexts.size(); i++ ) {
		ContextRuntimeInfoForElasticity ctx_runtime_info;
		ctx_runtime_info.contextName = contexts[i]->contextName;
		ctx_runtime_info.contextExecTime = contexts[i]->getCPUTime() * 0.000001;
		ctx_runtime_info.fromAccessCount = contexts[i]->getFromAccessCountByTypes(manageContextTypes);
		ctx_runtime_info.toAccessCount = contexts[i]->getToAccessCountByTypes(manageContextTypes);
		predictTotalClientRequests += ctx_runtime_info.getTotalClientRequestNumber();

		contextsRuntimeInfo[ contexts[i]->contextName ] = ctx_runtime_info;
		(contexts[i]->runtimeInfo).clear();
	}
	sl.unlock();


	ServerRuntimeInfo server_info( currentcpuUsage, total_cpu_time, contextMigrationThreshold, contexts.size(), predictTotalClientRequests );

	macedbg(1) << "Server's CPU usage: " << currentcpuUsage << Log::endl;
	
	_service->waitForServerInformationUpdate( mace::eMonitor::MAJOR_HANDLER_THREAD_ID, server_info );

	if( scaleOpType == ServersRuntimeInfo::NO_ACTION ){
		for( uint32_t i=0; i<contexts.size(); i++ ) {
			contexts[i] = NULL;
		}
		macedbg(1) << "LEM recv NO_ACTION from GEM!" << Log::endl;
		return;
	} else if( scaleOpType == ServersRuntimeInfo::SCALE_DOWN ){
		macedbg(1) << "LEM recv SCALE_DOWN from GEM, targetAddr: " << scaledownAddr << Log::endl;
		for( uint32_t i=0; i<contexts.size(); i++ ) {
			this->requestContextMigration( contexts[i]->contextName, scaledownAddr);
			contexts[i] = NULL;
		}
		return;		
	}
	
	const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
	macedbg(1) << "Have updated all servers' information: " << servers_info << Log::endl;

	this->processContextElasticityRules( marker, contexts );
	// this->processContextElasticityByInteraction( contexts );
	
	for( uint32_t i=0; i<contexts.size(); i++ ) {
		contexts[i] = NULL;
	}

	sl.lock();
	readyToProcessMigrationQuery = true;
	sl.unlock();

	processContextsMigrationQuery();
}

void mace::eMonitor::processContextElasticityPageRankOpt( const mace::string& marker, std::vector<mace::ContextBaseClass*>& contexts ){
	ADD_SELECTORS("eMonitor::processContextElasticityPageRankOpt");
	const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
	ContextService* _service = static_cast<ContextService*>(sv);

	mace::map< mace::MaceAddr, double > servers_cpu_usage;
	mace::map< mace::MaceAddr, double > servers_cpu_time;
	mace::map< mace::MaceAddr, mace::set<mace::string> > migration_ctx_sets;
	mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> > migrationContextsInfo;

	for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::const_iterator iter = servers_info.begin(); iter != servers_info.end(); 
			iter++) {
		servers_cpu_usage[ iter->first ] = (iter->second).CPUUsage;
		servers_cpu_time[ iter->first ] = (iter->second).totalCPUTime;
	}

	for( uint32_t i=0; i<contexts.size(); i++ ) {
		mace::ElasticityBehaviorAction* action = contexts[i]->getImprovePerformanceAction( _service->getLatestContextMapping(), 
			servers_info, marker, migration_ctx_sets, servers_cpu_usage, servers_cpu_time );
		if( action != NULL ){
			double ctx_exec_time = contexts[i]->getCPUTime() * 0.000001;

			mace::MigrationContextInfo migration_ctx_info( action->contextName, contexts[i]->getMarkerAvgLatency(marker) * 0.000001, 
				ctx_exec_time, contexts[i]->getContextsInterCount(), contexts[i]->getEstimateContextsInterSize(), contexts[i]->getMarkerCount(marker) );
			
			migrationContextsInfo[ action->destNode ].push_back(migration_ctx_info);
			migration_ctx_sets[ action->destNode ].insert( action->contextName );

			servers_cpu_usage[ action->destNode ] = 100 * ( servers_cpu_usage[action->destNode]*servers_cpu_time[action->destNode]*0.01 + ctx_exec_time ) / (servers_cpu_time[action->destNode]);
			servers_cpu_usage[ Util::getMaceAddr() ] = 100 * ( servers_cpu_usage[Util::getMaceAddr()]*servers_cpu_time[Util::getMaceAddr()]*0.01 - ctx_exec_time ) / (servers_cpu_time[Util::getMaceAddr()]);
		
		} else {
			predictLocalContexts.insert( contexts[i]->contextName );
		}
	}

	for( mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> >::iterator iter = migrationContextsInfo.begin(); 
			iter != migrationContextsInfo.end(); iter++ ){
		if( (iter->second).size() > 0 ) {
			_service->send__elasticity_contextMigrationQuery( iter->first, iter->second );
		}
	}
}

void mace::eMonitor::processContextElasticityPageRankIso( const mace::string& marker, 
		std::vector<mace::ContextBaseClass*>& contexts ) {
	ADD_SELECTORS("eMonitor::processContextElasticityPageRankIso");

	// const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getServersInfo();
	// const mace::vector<mace::ElasticityRule>& rules = eConfig->getElasticityRules();
	// ContextService* _service = static_cast<ContextService*>(sv);

	// const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	// mace::map< mace::MaceAddr, double > servers_cpu_usage;
	// mace::map< mace::MaceAddr, double > servers_cpu_time;
	// mace::map< mace::MaceAddr, mace::set<mace::string> > migration_ctx_sets;
	// mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> > migrationContextsInfo;

	// for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::const_iterator iter = servers_info.begin(); iter != servers_info.end(); 
	// 		iter++) {
	// 	servers_cpu_usage[ iter->first ] = (iter->second).CPUUsage;
	// 	servers_cpu_time[ iter->first ] = (iter->second).totalCPUTime;
	// }

	// for( uint64_t i=0; i<rules.size(); i++ ){
	// 	mace::vector< mace::string > satisfied_contexts = rules[i].satisfyGlobalRuleConditions(contexts);
	// 	const mace::ElasticityBehavior& behavior = rules[i].getElasticityBehavior();
	// 	for( uint64_t j=0; j<satisfied_contexts.size(); j++  ){
	// 		mace::ContextBaseClass* ctxObj = _service->getContextObjByName( satisfied_contexts[j] );
	// 		mace::ElasticityBehaviorAction* action = behavior.generateElasticityAction( ctxObj, snapshot, servers_info, 
	// 			servers_cpu_usage, servers_cpu_time, migration_ctx_sets, ctxObj->getMarkerCount(marker) );
	// 		if( action != NULL ){
	// 			double ctx_exec_time = ctxObj->getCPUTime() * 0.000001;

	// 			mace::MigrationContextInfo migration_ctx_info( action->contextName, ctxObj->getMarkerAvgLatency(marker) * 0.000001, 
	// 				ctx_exec_time, ctxObj->getContextsInterCount(), ctxObj->getEstimateContextsInterSize(), ctxObj->getMarkerCount(marker) );
	// 			migration_ctx_info.specialRequirement = mace::MigrationContextInfo::IDLE_CPU;
			
	// 			migrationContextsInfo[ action->destNode ].push_back(migration_ctx_info);
	// 			migration_ctx_sets[ action->destNode ].insert( action->contextName );

	// 			servers_cpu_usage[ action->destNode ] = 100 * ( servers_cpu_usage[action->destNode]*servers_cpu_time[action->destNode]*0.01 + ctx_exec_time ) / (servers_cpu_time[action->destNode]);
	// 			servers_cpu_usage[ Util::getMaceAddr() ] = 100 * ( servers_cpu_usage[Util::getMaceAddr()]*servers_cpu_time[Util::getMaceAddr()]*0.01 - ctx_exec_time ) / (servers_cpu_time[Util::getMaceAddr()]);
	// 		}
	// 	}
	// }

	// for( mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> >::iterator iter = migrationContextsInfo.begin(); 
	// 		iter != migrationContextsInfo.end(); iter++ ){
	// 	if( (iter->second).size() > 0 ) {
	// 		_service->send__elasticity_contextMigrationQuery( iter->first, iter->second );
	// 	}
	// }

}

void mace::eMonitor::processContextElasticityRules( const mace::string& marker, std::vector<mace::ContextBaseClass*>& contexts ) {
	ADD_SELECTORS("eMonitor::processContextElasticityRules");

	const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
	const mace::vector<mace::ElasticityRule>& rules = eConfig->getElasticityRules();
	ContextService* _service = static_cast<ContextService*>(sv);
	const ContextStructure& ownerships = _service->contextStructure; 

	const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	mace::map< mace::MaceAddr, double > servers_cpu_usage;
	mace::map< mace::MaceAddr, double > servers_cpu_time;
	mace::map< mace::MaceAddr, uint64_t > servers_creq_count;
	mace::map< mace::MaceAddr, mace::set<mace::string> > migration_ctx_sets;
	mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> > migrationContextsInfo;
	mace::set< mace::string > to_migrate_contexts;

	ServerRuntimeInfo local_server_info;

	for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::const_iterator iter = servers_info.begin(); iter != servers_info.end(); 
			iter++) {
		servers_cpu_usage[ iter->first ] = (iter->second).CPUUsage;
		servers_cpu_time[ iter->first ] = (iter->second).totalCPUTime;
		servers_creq_count[ iter->first ] = (iter->second).totalClientRequestNumber;
		if( iter->first == Util::getMaceAddr() ){
			local_server_info = iter->second;
		}
	}

	mace::vector<mace::string> context_names;
	for( uint32_t i=0; i<contexts.size(); i++ ){
		context_names.push_back(contexts[i]->contextName);
	}
    
    // check LEM rules
	for( uint64_t i=0; i<rules.size(); i++ ){
		if( rules[i].ruleType != mace::ElasticityRule::GLOBAL_RULE ){
			continue;
		} 

		mace::set< mace::string > satisfied_contexts = rules[i].satisfyGlobalRuleConditions(context_names, contextsRuntimeInfo, 
			local_server_info, ownerships );
		macedbg(1) << "satisfied_contexts: " << satisfied_contexts << Log::endl;
		mace::vector<mace::string> waiting_contexts;
		mace::set<mace::string> waiting_contexts_set;
		while(true) {
			if( waiting_contexts.size() == 0 ) {
				if( satisfied_contexts.size() > 0 ){
					mace::string ctx_name = *(satisfied_contexts.begin());
					waiting_contexts.push_back(ctx_name);
					waiting_contexts_set.insert(ctx_name);
					satisfied_contexts.erase(ctx_name);
				} else {
					break;
				}
			}

			ASSERT(waiting_contexts.size() > 0);
			mace::string next_context = waiting_contexts[0];
			waiting_contexts.erase(waiting_contexts.begin());
			macedbg(1) << "To process context: " << next_context << Log::endl;

			ASSERT( contextsRuntimeInfo.find(next_context) != contextsRuntimeInfo.end() );
			const ContextRuntimeInfoForElasticity& ctx_runtime_info = contextsRuntimeInfo[ next_context ];

			mace::ElasticityBehaviorAction* action = rules[i].generateElasticityAction( ctx_runtime_info, snapshot, ownerships, 
				servers_info, servers_cpu_usage, servers_cpu_time, servers_creq_count, migration_ctx_sets );

			if( action != NULL && to_migrate_contexts.count(action->contextName) == 0 ){
				double ctx_exec_time = ctx_runtime_info.contextExecTime;

				mace::MigrationContextInfo migration_ctx_info( action->contextName, ctx_runtime_info.avgLatency, 
					ctx_exec_time, ctx_runtime_info.fromAccessCount, ctx_runtime_info.fromMessageSize, ctx_runtime_info.count );
				migration_ctx_info.specialRequirement = action->specialRequirement;

				migrationContextsInfo[ action->destNode ].push_back(migration_ctx_info);
				migration_ctx_sets[ action->destNode ].insert( action->contextName );
				to_migrate_contexts.insert( action->contextName );

				servers_cpu_usage[ action->destNode ] = 100 * ( servers_cpu_usage[action->destNode]*servers_cpu_time[action->destNode]*0.01 + ctx_exec_time ) / (servers_cpu_time[action->destNode]);
				servers_cpu_usage[ Util::getMaceAddr() ] = 100 * ( servers_cpu_usage[Util::getMaceAddr()]*servers_cpu_time[Util::getMaceAddr()]*0.01 - ctx_exec_time ) / (servers_cpu_time[Util::getMaceAddr()]);
			

				servers_creq_count[ action->destNode ] += ctx_runtime_info.getTotalClientRequestNumber();
				servers_creq_count[ Util::getMaceAddr() ] -= ctx_runtime_info.getTotalClientRequestNumber();
			
				mace::set<mace::string> strong_conn_contexts = ctx_runtime_info.getStrongConnectContexts();
				for( mace::set<mace::string>::iterator iter = strong_conn_contexts.begin(); iter != strong_conn_contexts.end(); iter ++ ){
					if( waiting_contexts_set.count(*iter) == 0 ){
						waiting_contexts_set.insert(*iter);
						waiting_contexts.push_back(*iter);
						macedbg(1) << "Add context("<< next_context <<")'s strong connected context("<< *iter <<")!" << Log::endl;
					}
					satisfied_contexts.erase(*iter);
				}				
			}
		}
	}

	// check context rules
	// for( uint32_t i=0; i<contexts.size(); i++ ){
	// 	mace::ElasticityBehaviorAction* action = contexts[i]->proposeAndCheckMigrationAction( snapshot, marker, servers_info, servers_cpu_usage, 
	// 																						servers_cpu_time, migration_ctx_sets);
		
	// 	if( action != NULL ){
	// 		double ctx_exec_time = contexts[i]->getCPUTime() * 0.000001;

	// 		mace::MigrationContextInfo migration_ctx_info = action->generateMigrationContextInfo( 
	// 																ctx_exec_time, contexts[i]->getContextsInterCount(), 
	// 																contexts[i]->getEstimateContextsInterSize(),
	// 																contexts[i]->getMarkerCount(marker) );

	// 		migrationContextsInfo[ action->destNode ].push_back(migration_ctx_info);
	// 		migration_ctx_sets[ action->destNode ].insert( action->contextName );
	// 		to_migrate_contexts.insert( action->contextName );

	// 		servers_cpu_usage[ action->destNode ] = 100 * ( servers_cpu_usage[action->destNode]*servers_cpu_time[action->destNode]*0.01 + ctx_exec_time ) / (servers_cpu_time[action->destNode]);
	// 		servers_cpu_usage[ Util::getMaceAddr() ] = 100 * ( servers_cpu_usage[Util::getMaceAddr()]*servers_cpu_time[Util::getMaceAddr()]*0.01 - ctx_exec_time ) / (servers_cpu_time[Util::getMaceAddr()]);
	// 		delete action;
	// 		action = NULL;
	// 	}
	// }

	for( mace::map< mace::MaceAddr, mace::vector<mace::MigrationContextInfo> >::iterator iter = migrationContextsInfo.begin(); 
			iter != migrationContextsInfo.end(); iter++ ){
		if( (iter->second).size() > 0 ) {
			_service->send__elasticity_contextMigrationQuery( iter->first, iter->second );
		}
	}

}

void mace::eMonitor::processContextElasticityByInteraction( std::vector<mace::ContextBaseClass*>& contexts ) {
	ADD_SELECTORS("eMonitor::processContextElasticityByInteraction");
	exchangeContexts.clear();
	exchangeBenefits.clear();

	ContextService* _service = static_cast<ContextService*>(sv);
	const mace::set<mace::MaceAddr>& server_addrs = serversRuntimeInfo.getActiveServerAddrs();
	const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	for( mace::set<mace::MaceAddr>::const_iterator iter = server_addrs.begin(); iter != server_addrs.end(); iter ++ ) {
		if( *iter == Util::getMaceAddr() ){
			continue;
		}
		exchangeBenefits[ *iter ] = 0;
	}

	for( std::map< mace::string, ContextRuntimeInfoForElasticity >::iterator iter1 = contextsRuntimeInfo.begin();
			iter1 != contextsRuntimeInfo.end(); iter1 ++ ) {
		macedbg(1) << "Compute exchange benefit for context " << iter1->first << Log::endl;
		mace::map< mace::MaceAddr, uint64_t > benefit = (iter1->second).computeExchangeBenefit(snapshot);
		for( mace::map< mace::MaceAddr, uint64_t>::iterator iter2 = benefit.begin(); iter2 != benefit.end(); iter2 ++ ) {
			exchangeBenefits[iter2->first] += iter2->second;
			mace::pair< mace::string, uint64_t > exchange_context(iter1->first, iter2->second);
			exchangeContexts[iter2->first].push_back( exchange_context );
		}
	}

	this->processNextContextsExchangeRequest();
}

void mace::eMonitor::collectContextsInfos(const uint64_t& r) {
	ADD_SELECTORS("eMonitor::collectContextsInfos");
	// macedbg(1) << "To collect contexts runtime information!" << Log::endl;

	bool flag = true;

	currentcpuUsage = this->computeCurrentCPUUsage();

	ContextService* _service = static_cast<ContextService*>(sv);

	// uint64_t uint_cpu_usage = (uint64_t)(currentcpuUsage * 100);
	macedbg(1) << "Server's CPU usage: " << currentcpuUsage << Log::endl;
	// _service->send__elasticity_informationReport(uint_cpu_usage);
	

	std::vector<mace::ContextBaseClass*> contexts;
	_service->getContextObjectsByTypes(contexts, manageContextTypes);

	std::vector<mace::ElasticityBehaviorEvent*> lem_ebevents;
	// for( uint32_t i=0; i<contexts.size(); i++ ){
	// 	std::vector<mace::ElasticityBehaviorEvent*> ebEvents = contexts[i]->checkElasticityRules();
	// 	for( uint32_t j=0; j<ebEvents.size(); j++ ) {
	// 		lem_ebevents.push_back( ebEvents[j] );
	// 	}
	// 	(contexts[i]->runtimeInfo).clear();
	// 	contexts[i] = NULL;
	// }

	uint64_t total_ctx_execution_time = 0;
	for( uint32_t i=0; i<contexts.size(); i++ ){
		total_ctx_execution_time += contexts[i]->getCPUTime();
	}

	if( total_ctx_execution_time == 0 ) {
		// macedbg(1) << "Total CPU execution time is " << total_ctx_execution_time << Log::endl;
		return;
	}

	const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
	for( uint32_t i=0; i<contexts.size(); i++ ){
		(contexts[i]->runtimeInfo).printRuntimeInformation( total_ctx_execution_time, currentcpuUsage, snapshot, r );
		(contexts[i]->runtimeInfo).clear();
		contexts[i] = NULL;
	}

	if( flag ){
		return;
	}

	// Group elasticity behaviors
	uint32_t set_id = 1;
	std::map<uint32_t, std::vector<mace::ElasticityBehaviorEvent*> > eb_sets;
	std::map<uint32_t, mace::set<mace::string> > context_sets;

	std::map<uint32_t, std::vector<mace::ElasticityBehaviorEvent*> >::iterator eb_iter;
	std::map<uint32_t, mace::set<mace::string> >::iterator ctx_iter;

	for( uint32_t i=0; i<lem_ebevents.size(); i++ ) {
		mace::ElasticityBehaviorEvent* ebevent = lem_ebevents[i];

		mace::set<mace::string> contexts = ebevent->getContexts();

		mace::set<uint32_t> set_ids;

		for( ctx_iter=context_sets.begin(); ctx_iter!=context_sets.end(); ctx_iter++ ) {
			for( mace::set<mace::string>::iterator iter=contexts.begin(); iter!=contexts.end(); iter++ ) {
				if( (ctx_iter->second).count(*iter) > 0 ) {
					set_ids.insert( ctx_iter->first );
					break;
				}
			}
		}

		if( set_ids.size() == 0 ){
			eb_sets[set_id].push_back( ebevent );
			for( mace::set<mace::string>::iterator iter=contexts.begin(); iter!=contexts.end(); iter++ ) {
				context_sets[set_id].insert( *iter );
			}
			set_id ++;
		} else { // combine related sets
			mace::set<uint32_t>::iterator id_iter = set_ids.begin();
			uint32_t major_set_id = *id_iter;

			std::vector<mace::ElasticityBehaviorEvent*>& major_eb_set = eb_sets[major_set_id];
			mace::set<mace::string>& major_ctx_set = context_sets[major_set_id];

			for(; id_iter != set_ids.end(); id_iter ++ ) {
				if( *id_iter == major_set_id ){
					continue;
				}

				std::vector<mace::ElasticityBehaviorEvent*>& eb_set = eb_sets[ *id_iter ];
				for( std::vector<mace::ElasticityBehaviorEvent*>::iterator e_iter=eb_set.begin(); e_iter!=eb_set.end(); e_iter++ ) {
					major_eb_set.push_back(*e_iter);
				}

				mace::set<mace::string>& ctx_set = context_sets[ *id_iter ];
				for( mace::set<mace::string>::iterator c_iter=ctx_set.begin(); c_iter!=ctx_set.end(); c_iter++ ){
					major_ctx_set.insert(*c_iter);
				}

				eb_sets.erase( *id_iter );
				context_sets.erase( *id_iter );
			}

			major_eb_set.push_back( ebevent );
			for( mace::set<mace::string>::iterator iter=contexts.begin(); iter!=contexts.end(); iter++ ) {
				if( major_ctx_set.count(*iter) == 0 ) {
					major_ctx_set.insert(*iter);
				}
			}
		}
	}

	// Generating valid sequential actions
	for( eb_iter=eb_sets.begin(); eb_iter!=eb_sets.end(); eb_iter++ ) {
		std::vector<mace::ElasticityBehaviorEvent*>& eb_vector = eb_iter->second;

		std::vector< std::vector<mace::ElasticityBehaviorAction*> > eb_actions;
		mace::vector< uint32_t > action_vector_iter;
		for( uint32_t i=0; i<eb_vector.size(); i++ ) {
			mace::ElasticityBehaviorEvent* ebevent = eb_vector[i];
			std::vector<mace::ElasticityBehaviorAction*> eb_action = this->generateActionsForElasticityBehavior( ebevent );
			eb_actions.push_back( eb_action );
			action_vector_iter.push_back(0);
		}

		std::vector< std::vector<mace::ElasticityBehaviorAction*> > valid_eb_actions;
		while(true) {
			std::vector< mace::ElasticityBehaviorAction*> seq_action;
			for( uint32_t i=0; i<action_vector_iter.size(); i++ ){
				seq_action.push_back( eb_actions[i][ action_vector_iter[i] ] );
			}
			if( this->isValidSequentialActions(seq_action) ){
				valid_eb_actions.push_back( seq_action );
			}

			uint32_t i=0;
			bool done_flag = false;
			while(true){
				action_vector_iter[i] = action_vector_iter[i]+1;
				if( action_vector_iter[i] == eb_actions[i].size() ){
					if( i == eb_actions.size()-1 ){
						done_flag = true;
						break;
					} else {
						action_vector_iter[i] = 0;
						i ++;
					}
				} else {
					break;
				}
			}

			if( done_flag ) {
				break;
			}
		}

		uint32_t best_seq_action_iter = 0;
		double best_seq_action_score = 0.0;

		for( uint32_t i=0; i<valid_eb_actions.size(); i++ ){
			double score = this->computeSeqentialActionScore( valid_eb_actions[i] );
			if( score > best_seq_action_score ){
				best_seq_action_iter = i;
				best_seq_action_score = score;
			}
		}

		std::vector<mace::ElasticityBehaviorAction*>& best_seq_action = valid_eb_actions[best_seq_action_iter];
		for( uint32_t i=0; i<best_seq_action.size(); i++ ){
			mace::eMonitor::enqueueElasticityBehaviorAction( best_seq_action[i] );
		}
	}
}

void mace::eMonitor::updateServerInfo(const mace::MaceAddr& src, const mace::ServerRuntimeInfo& server_info ){
	ADD_SELECTORS("eMonitor::updateServerInfo");
	if( serversRuntimeInfo.updateServerInfo(src, server_info ) && isGEM ) {
		ContextService* _service = static_cast<ContextService*>(sv);
		mace::set< mace::MaceAddr > skip_addrs;
		mace::map< mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
 		
 		// adjust the scale of the applications
		// double total_cpu_usage;

		// servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);

		// for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo>::const_iterator iter = servers_info.begin(); 
		// 		iter != servers_info.end(); iter ++ ){
		// 	total_cpu_usage += (iter->second).CPUUsage;
		// }

		// double avg_cpu_usage = total_cpu_usage / servers_info.size();
		
		// if( avg_cpu_usage > CPU_BUSY_THREAHOLD ){
		// 	serversRuntimeInfo.scaleup(1);
		// } else if( avg_cpu_usage < CPU_IDLE_THREAHOLD ){
		// 	mace::vector< mace::pair<MaceAddr, MaceAddr> > scaledown_servers = serversRuntimeInfo.scaledown();
		// 	mace::map< mace::MaceAddr, mace::ServerRuntimeInfo> servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
		// 	for( uint32_t i=0; i<scaledown_servers.size(); i++ ) {
		// 		const mace::pair<MaceAddr, MaceAddr> sd_server = scaledown_servers[i];
		// 		_service->send__elasticity_serverInfoReply( sd_server.first, mace::eMonitor::MAJOR_HANDLER_THREAD_ID, servers_info, 
		// 			mace::ServersRuntimeInfo::SCALE_DOWN, sd_server.second );
		// 		_service->send__elasticity_serverInfoReply( sd_server.second, mace::eMonitor::MAJOR_HANDLER_THREAD_ID, servers_info, 
		// 			mace::ServersRuntimeInfo::NO_ACTION, sd_server.second );

		// 		skip_addrs.insert( sd_server.first );
		// 		skip_addrs.insert( sd_server.second );
		// 	} 
		// }

		const mace::set<mace::MaceAddr>& servers = serversRuntimeInfo.getServerAddrs();
		servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);

		for( mace::set<mace::MaceAddr>::const_iterator iter = servers.begin(); iter != servers.end(); iter ++ ){
			if( skip_addrs.count(*iter) > 0 ) {
				continue;
			}

			if( servers_info.find(*iter) == servers_info.end() ) {
				_service->send__elasticity_serverInfoReply( *iter, mace::eMonitor::MAJOR_HANDLER_THREAD_ID, servers_info, 
					mace::ServersRuntimeInfo::NO_ACTION, *iter );
			} else {
				_service->send__elasticity_serverInfoReply( *iter, mace::eMonitor::MAJOR_HANDLER_THREAD_ID, servers_info, 
					mace::ServersRuntimeInfo::NORMAL, *iter );
			}
			
		}
	}
}

void mace::eMonitor::enqueueContextMigrationQuery( const mace::MaceAddr& src, const mace::vector<mace::MigrationContextInfo>& m_contexts_info ) {
	ADD_SELECTORS("eMonitor::enqueueContextMigrationQuery");
	ScopedLock sl(migrationMutex);
	macedbg(1) << "Receive context(s) migration query from " << src << Log::endl;
	ContextMigrationQuery query(src, m_contexts_info);
	contextMigrationQueries.push_back( query );
	sl.unlock();

	processContextsMigrationQuery();
}

void mace::eMonitor::processContextsMigrationQuery() {
	ADD_SELECTORS("eMonitor::processContextsMigrationQuery");
	// bool flag = true;
	// this->processExchangeContextsQuery();
	// if( flag ){	
	// 	return;
	// }
	
	const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
	ScopedLock sl(migrationMutex);
	while( contextMigrationQueries.size() > 0 && readyToProcessMigrationQuery && migrationQueryThreadIsIdle ){
		ContextMigrationQuery query = contextMigrationQueries[0];
		contextMigrationQueries.erase( contextMigrationQueries.begin() );
		migrationQueryThreadIsIdle = false;
		sl.unlock();

		ContextService* _service = static_cast<ContextService*>(sv);
		const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

		const mace::MaceAddr& src = query.srcAddr;
		const mace::vector<mace::MigrationContextInfo>& m_contexts_info = query.migrationContextsInfo;

		mace::vector<mace::string> accept_m_ctxs;
		for( mace::vector<mace::MigrationContextInfo>::const_iterator iter = m_contexts_info.begin(); iter != m_contexts_info.end();
				iter++ ) {
			const mace::MigrationContextInfo& m_ctx_info = *iter;
			macedbg(1) << "Query: " << m_ctx_info << " from " << src << Log::endl;

			double accept_cpu_usage = 100 * ( predictCPUTime + m_ctx_info.contextExecTime ) / ( predictCPUTotalTime );
			macedbg(1) << "accept_cpu_usage=" << accept_cpu_usage << Log::endl;
			if( accept_cpu_usage >= mace::eMonitor::CPU_BUSY_THREAHOLD ) {
				continue;
			}
			double src_cpu_usage = serversRuntimeInfo.getServerCPUUsage( src );



			if( m_ctx_info.specialRequirement == mace::MigrationContextInfo::IDLE_CPU ) {
				// double migrated_src_cpu_usage = src_cpu_usage - 100 * ( m_ctx_info.contextExecTime / serversRuntimeInfo.getServerCPUTime(src) ); 
				macedbg(1) << "context("<< m_ctx_info.contextName <<") src_cpu_usage=" << src_cpu_usage << Log::endl;
				if( src_cpu_usage > accept_cpu_usage ) {
					predictCPUTime += m_ctx_info.contextExecTime;
					accept_m_ctxs.push_back( m_ctx_info.contextName );
				}
				
			} else if( m_ctx_info.specialRequirement == mace::MigrationContextInfo::BUSY_CPU ) {
				if( accept_cpu_usage < mace::eMonitor::CPU_BUSY_THREAHOLD ){
					predictCPUTime += m_ctx_info.contextExecTime;
					accept_m_ctxs.push_back( m_ctx_info.contextName );
				}
				
			} else if( m_ctx_info.specialRequirement == mace::MigrationContextInfo::IDLE_NET ) {
				uint64_t src_creq_count = serversRuntimeInfo.getServerClienRequestNumber( src );
				uint64_t ctx_creq_count = m_ctx_info.getTotalClientRequestNumber();
				uint64_t accept_creq_count = predictTotalClientRequests + ctx_creq_count;

				macedbg(1) << "context("<< m_ctx_info.contextName <<") src_creq_count=" << src_creq_count << ", accept_creq_count=" << accept_creq_count << Log::endl;
				if( src_creq_count > accept_creq_count ) {
					accept_m_ctxs.push_back( m_ctx_info.contextName );

					predictCPUTime += m_ctx_info.contextExecTime;
					predictTotalClientRequests += ctx_creq_count;
				}
				
			} else {

				const mace::map< mace::string, uint64_t>& ctxs_inter_count = m_ctx_info.contextsInterCount;
				const mace::map< mace::string, uint64_t>& ctxs_inter_size = m_ctx_info.contextsInterSize;

				double local_msg_count1 = 0.0;
  				double remote_msg_size1 = 0.0;

  				double local_msg_count2 = 0.0;
				double remote_msg_size2 = 0.0;
				for( mace::map< mace::string, uint64_t>::const_iterator c_iter = ctxs_inter_count.begin(); c_iter != ctxs_inter_count.end(); 
						c_iter++ ){

					mace::map< mace::string, uint64_t>::const_iterator s_iter = ctxs_inter_size.find(c_iter->first);
					ASSERT( s_iter != ctxs_inter_size.end() );
					if( predictLocalContexts.count( c_iter->first) > 0 ){
						local_msg_count2 += c_iter->second;
						remote_msg_size1 += s_iter->second;
					} else {
						remote_msg_size2 += s_iter->second;
						if( mace::ContextMapping::getNodeByContext(snapshot, c_iter->first) == src ) {
							local_msg_count1 += c_iter->second;
						} else {
							remote_msg_size1 += s_iter->second;
						}
					}
				}

				if( m_ctx_info.count > 1 ){
					local_msg_count1 /= m_ctx_info.count;
					local_msg_count2 /= m_ctx_info.count;
					remote_msg_size1 /= m_ctx_info.count;
					remote_msg_size2 /= m_ctx_info.count;
				}

				mace::map<mace::MaceAddr, ServerRuntimeInfo>::const_iterator sinfo_iter = servers_info.find(src);
          		ASSERT( sinfo_iter != servers_info.end() );

				if( m_ctx_info.specialRequirement == mace::MigrationContextInfo::MAX_LATENCY ) {
					// double expect_benefit = m_ctx_info.currLatency - m_ctx_info.threshold;

					double ctx_cpu_usage = m_ctx_info.contextExecTime*100 / (sinfo_iter->second).totalCPUTime;

          			double est_benefit_percent1 = mace::eMonitor::estimateBenefitPercent( 0, ctx_cpu_usage, src_cpu_usage, 
            			local_msg_count1, remote_msg_size1, accept_cpu_usage, local_msg_count2, remote_msg_size2 );

          		// double est_benefit_percent2 = mace::eMonitor::estimateBenefitPercent( expect_benefit, ctx_cpu_usage, src_cpu_usage, 
            // 		local_msg_count1, remote_msg_size1, accept_cpu_usage, local_msg_count2, remote_msg_size2 );

          			double est_benefit_percent2 = mace::eMonitor::estimateBenefitPercent( 0, ctx_cpu_usage, accept_cpu_usage, 
            			local_msg_count2, remote_msg_size2, src_cpu_usage, local_msg_count1, remote_msg_size1 );

          			if( m_ctx_info.currLatency < m_ctx_info.threshold ){
            			est_benefit_percent1 = 1.0 - est_benefit_percent1;
            			est_benefit_percent2 = 1.0 - est_benefit_percent2;
          			}

          			double est_benefit_percent = est_benefit_percent1 - est_benefit_percent2;
          			double migration_threshold = (sinfo_iter->second).contextMigrationThreshold;

          			macedbg(1) << "context("<< m_ctx_info.contextName <<"): migration_threshold=" << migration_threshold << ", est_benefit_percent1=" << est_benefit_percent1 << ", est_benefit_percent2=" << est_benefit_percent2 << Log::endl;

          			if( est_benefit_percent >= migration_threshold /*&& est_benefit_percent2 < migration_threshold */) {
          				accept_m_ctxs.push_back(m_ctx_info.contextName);
						predictCPUTime += m_ctx_info.contextExecTime;
						MigratedContextInfo md_ctx_info( m_ctx_info.currLatency, m_ctx_info.benefit);
						migratedContextsInfo[ m_ctx_info.contextName ] = md_ctx_info;
          			} 
				}
			}
		
		}
		if( accept_m_ctxs.size() > 0 ){
			ContextService* _service = static_cast<ContextService*>(sv);
			_service->send__elasticity_contextMigrationQueryReply( src, accept_m_ctxs);
		}
		
		sl.lock();
		migrationQueryThreadIsIdle = true;
	}
}

void mace::eMonitor::processExchangeContextsQuery() {
	ADD_SELECTORS("eMonitor::processExchangeContextsQuery");
	ContextService* _service = static_cast<ContextService*>(sv);
	mace::vector<mace::string> reject_ctxes;
	reject_ctxes.push_back("#");
	const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info = serversRuntimeInfo.getActiveServersInfo(isGEM);
	macedbg(1) << "Waiting request number: " << contextMigrationQueries.size() << Log::endl;
	ScopedLock sl(migrationMutex);
	while( contextMigrationQueries.size() > 0 && readyToProcessMigrationQuery && migrationQueryThreadIsIdle ){
		ContextMigrationQuery query = contextMigrationQueries[0];
		contextMigrationQueries.erase( contextMigrationQueries.begin() );
		macedbg(1) << "To process request from " << query.srcAddr << ", exchangeProcessStep=" << exchangeProcessStep << Log::endl;
		if( exchangeProcessStep == 2 ) {
			macedbg(1) << "Reject request from " << query.srcAddr << Log::endl;
			_service->send__elasticity_contextMigrationQueryReply( query.srcAddr, reject_ctxes );
			continue;
		}else if( exchangeProcessStep == 1 ) { // waiting for final reply
			macedbg(1) << "Postpone request from " << query.srcAddr << Log::endl;
			return;
		}

		migrationQueryThreadIsIdle = false;
		sl.unlock();

		const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

		const mace::MaceAddr& src = query.srcAddr;
		mace::vector<mace::MigrationContextInfo> s_ctxes = query.migrationContextsInfo;

		for( uint32_t i=0; i<s_ctxes.size(); i++ ) {
			macedbg(1) << "Accept context("<< s_ctxes[i].contextName <<") with benefit("<< s_ctxes[i].benefit <<")!" << Log::endl;
		}

		mace::vector<mace::MigrationContextInfo> t_ctxes;

		if( exchangeContexts.find(src) != exchangeContexts.end() ) {
			const mace::vector< mace::pair<mace::string, uint64_t> >& ctxes = exchangeContexts[src];
			for( mace::vector< mace::pair<mace::string, uint64_t> >::const_iterator iter = ctxes.begin(); iter != ctxes.end();
					iter ++ ) {
				const mace::pair< mace::string, uint64_t >& exchange_context = *iter;
				ASSERT( contextsRuntimeInfo.find(exchange_context.first) != contextsRuntimeInfo.end() );
				MigrationContextInfo migration_ctx_info( exchange_context.first, contextsRuntimeInfo[exchange_context.first].currLatency, 
												contextsRuntimeInfo[exchange_context.first].contextExecTime, 
												contextsRuntimeInfo[exchange_context.first].fromAccessCount,
												contextsRuntimeInfo[exchange_context.first].fromMessageSize,
												contextsRuntimeInfo[exchange_context.first].count );
				migration_ctx_info.benefit = exchange_context.second;
				macedbg(1) << "Exchange context("<< migration_ctx_info.contextName <<") with benefit("<< migration_ctx_info.benefit <<")!" << Log::endl;
				
				bool insert_flag = false;
				for( mace::vector<mace::MigrationContextInfo>::iterator t_iter = t_ctxes.begin(); t_iter != t_ctxes.end();
						t_iter++ ) {
					if( (*t_iter).benefit < (*iter).second ) {
						t_ctxes.insert( t_iter, migration_ctx_info );
						insert_flag = true;
						break;
					}
				}
				if( !insert_flag ){
					t_ctxes.push_back( migration_ctx_info );
				}
			}
		}

		mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>::const_iterator si_iter = servers_info.find(src);
		ASSERT( si_iter != servers_info.end() );
		int src_n_context = (si_iter->second).contextsNumber;
		
		si_iter = servers_info.find(Util::getMaceAddr());
		ASSERT( si_iter != servers_info.end() );
		int local_n_context = (si_iter->second).contextsNumber;

		mace::vector<mace::string> accept_ctxes;
		mace::vector<mace::string> exchange_ctxes;

		int min_n_ctx = src_n_context;
		if( min_n_ctx > local_n_context ) {
			min_n_ctx = local_n_context;
		}

		int max_n_diff_ctx = (int)(min_n_ctx * eMonitor::CONEXT_NUMBER_BALANCE_THRESHOLD );
		if( max_n_diff_ctx < eMonitor::MIN_CONEXT_NUMBER_BALANCE ) {
			max_n_diff_ctx = eMonitor::MIN_CONEXT_NUMBER_BALANCE;
		}

		macedbg(1) << "max_n_diff_ctx=" << max_n_diff_ctx << Log::endl;
		while(true) {
			mace::string ctx_name;
			macedbg(1) << "src_n_context=" << src_n_context << ", local_n_context=" << local_n_context << Log::endl;
			if( src_n_context-local_n_context > max_n_diff_ctx ) {
				if( s_ctxes.size() == 0 ) {
					break;
				}
				ctx_name = (*(s_ctxes.begin())).contextName;
				s_ctxes.erase( s_ctxes.begin() ); 
				src_n_context --;
				local_n_context ++;
				accept_ctxes.push_back(ctx_name);
			} else if( local_n_context-src_n_context > max_n_diff_ctx ) {
				if( t_ctxes.size() == 0 ) {
					break;
				}
				ctx_name = (*(t_ctxes.begin())).contextName;
				t_ctxes.erase( t_ctxes.begin() ); 
				src_n_context ++;
				local_n_context --;
				exchange_ctxes.push_back(ctx_name);
			} else if( s_ctxes.size() == 0 ) {
				if( t_ctxes.size() == 0 ) {
					break;
				}
				ctx_name = (*(t_ctxes.begin())).contextName;
				t_ctxes.erase( t_ctxes.begin() ); 
				src_n_context ++;
				local_n_context --;
				exchange_ctxes.push_back(ctx_name);
			} else if( t_ctxes.size() == 0 ){
				if( s_ctxes.size() == 0 ) {
					break;
				}
				ctx_name = (*(s_ctxes.begin())).contextName;
				s_ctxes.erase( s_ctxes.begin() ); 
				src_n_context --;
				local_n_context ++;
				accept_ctxes.push_back(ctx_name);
			} else {
				if( (*(s_ctxes.begin())).benefit > (*(t_ctxes.begin())).benefit ) {
					ctx_name = (*(s_ctxes.begin())).contextName;
					s_ctxes.erase( s_ctxes.begin() ); 
					src_n_context --;
					local_n_context ++;
					accept_ctxes.push_back(ctx_name);
				} else {
					ctx_name = (*(t_ctxes.begin())).contextName;
					t_ctxes.erase( t_ctxes.begin() ); 
					src_n_context ++;
					local_n_context --;
					exchange_ctxes.push_back(ctx_name);
				}
			}
			macedbg(1) << "Choose context " << ctx_name << Log::endl;
			for( uint32_t i=0; i<s_ctxes.size(); i++ ) {
				s_ctxes[i].updateBenefitForInteraction(ctx_name, snapshot);
			}

			for( uint32_t i=0; i<t_ctxes.size(); i++ ) {
				t_ctxes[i].updateBenefitForInteraction(ctx_name, snapshot);
			}

			mace::eMonitor::resortMigrationContextsInfo( s_ctxes );
			mace::eMonitor::resortMigrationContextsInfo( t_ctxes );
		}

		macedbg(1) << "accept_ctxes: " << accept_ctxes << "; exchange_ctxes: " << exchange_ctxes << "; exchangeProcessStep: " << exchangeProcessStep << Log::endl;

		mace::vector<mace::string> ctxes;
		for( uint32_t i=0; i<accept_ctxes.size(); i++ ){
			ctxes.push_back( accept_ctxes[i] );
		}
		ctxes.push_back("#");
		for( uint32_t i=0; i<exchange_ctxes.size(); i++ ) {
			ctxes.push_back( exchange_ctxes[i] );
		}

		sl.lock();
		if( exchangeProcessStep == 2 ) {
			macedbg(1) << "The LEM already agree to exchange contexts with another LEM!" << Log::endl;
			_service->send__elasticity_contextMigrationQueryReply( src, reject_ctxes );
			migrationQueryThreadIsIdle = true;
			return;
		}

		if( ctxes.size() > 1 ){ // accept exchange contexts
			macedbg(1) << "The LEM agree to exchange contexts with " << src << Log::endl;
			exchangeProcessStep = 1;
			waitingReplyAddr = src;
		}	
		migrationQueryThreadIsIdle = true;
		sl.unlock();

		ContextService* _service = static_cast<ContextService*>(sv);
		_service->send__elasticity_contextMigrationQueryReply( src, ctxes );

		sl.lock();
	}
}

void mace::eMonitor::processContextsMigrationQueryReply( const mace::MaceAddr& dest, 
		const mace::vector<mace::string>& accept_m_contexts ) {
	ADD_SELECTORS("eMonitor::processContextsMigrationQueryReply");
	// this->processExchangeContextsQueryReply( dest, accept_m_contexts );

	for( uint64_t i=0; i<accept_m_contexts.size(); i++ ){
		this->requestContextMigration( accept_m_contexts[i], dest );
	}
}

void mace::eMonitor::processNextContextsExchangeRequest() {
	ADD_SELECTORS("eMonitor::processNextContextsExchangeRequest");
	ScopedLock sl(migrationMutex);
	// Get the largest benefit from existing servers
	mace::MaceAddr exchange_target_addr;
	uint64_t largest_benefit = 0;
	for( mace::map< mace::MaceAddr, uint64_t >::iterator iter = exchangeBenefits.begin(); iter != exchangeBenefits.end(); iter ++ ) {
		if( iter->second > largest_benefit ) {
			exchange_target_addr = iter->first;
			largest_benefit = iter->second;
		}
	}
	macedbg(1) << "Server("<< exchange_target_addr <<") exchange_benefit=" << largest_benefit << Log::endl;

	if( largest_benefit == 0 ){
		return;
	}

	exchangeBenefits.erase(exchange_target_addr);
	sl.unlock();

	ContextService* _service = static_cast<ContextService*>(sv);

	const mace::vector< mace::pair<mace::string, uint64_t> >& exchange_context_set = exchangeContexts[exchange_target_addr];
	mace::vector<mace::MigrationContextInfo> migration_ctxes_info;
	for( uint32_t i=0; i<exchange_context_set.size(); i++ ) {
		const mace::pair<mace::string, uint64_t>& exchange_context = exchange_context_set[i];
		ASSERT( contextsRuntimeInfo.find(exchange_context.first) != contextsRuntimeInfo.end() );
		MigrationContextInfo migration_ctx_info( exchange_context.first, contextsRuntimeInfo[exchange_context.first].currLatency, 
												contextsRuntimeInfo[exchange_context.first].contextExecTime, 
												contextsRuntimeInfo[exchange_context.first].fromAccessCount,
												contextsRuntimeInfo[exchange_context.first].fromMessageSize,
												contextsRuntimeInfo[exchange_context.first].count );
		migration_ctx_info.benefit = exchange_context.second;
		bool insert_flag = false;
		for( mace::vector<mace::MigrationContextInfo>::iterator iter = migration_ctxes_info.begin(); iter != migration_ctxes_info.end();
				iter ++ ) {
			if( (*iter).benefit < migration_ctx_info.benefit ) {
				migration_ctxes_info.insert( iter, migration_ctx_info );
				insert_flag = true;
				break;
			}
		}
		if( !insert_flag ) {
			migration_ctxes_info.push_back(migration_ctx_info);
		}
	}


	if( migration_ctxes_info.size() > 0 ) {
		macedbg(1) << "The LEM sends exchange requests to " << exchange_target_addr << Log::endl;
		_service->send__elasticity_contextMigrationQuery( exchange_target_addr, migration_ctxes_info );
	}
}

void mace::eMonitor::processExchangeContextsQueryReply( const mace::MaceAddr& dest, 
		const mace::vector<mace::string>& contexts ) {
	ADD_SELECTORS("eMonitor::processExchangeContextsQueryReply");
	// contexts inlcude either "#" (second step), "*" (third step agree) or none (third step reject)
	macedbg(1) << "Receive reply("<< contexts <<") from " << dest << ", exchangeProcessStep=" << (uint16_t)exchangeProcessStep << Log::endl;

	mace::vector<mace::string> to_migrate_contexts;
	mace::vector<mace::string> to_accept_contexts;

	bool migrate_ctx_flag = false;
	bool star_flag = false;
	for( uint32_t i=0; i<contexts.size(); i++ ) {
		if( contexts[i] == "#" ) {
			migrate_ctx_flag = true;
		} else if( contexts[i] == "*") {
			star_flag = true;
		} else if( !migrate_ctx_flag ) {
			to_migrate_contexts.push_back( contexts[i] );
		} else {
			to_accept_contexts.push_back( contexts[i] );
		}
	}

	ContextService* _service = static_cast<ContextService*>(sv);

	ScopedLock sl( migrationMutex );
	if( exchangeProcessStep == 0 ) {
		if( star_flag ) { // agree for final agreement
			ASSERTMSG(false, "LEM does not agree any request!");
		} else if( migrate_ctx_flag ) {
			if( contexts.size() == 1 ) {
				sl.unlock();
				macedbg(1) << "LEM picks up the next exchange request!" << Log::endl;
				processNextContextsExchangeRequest();
			} else {
				exchangeProcessStep = 2;
				sl.unlock();
				macedbg(1) << "LEM agree the final request from " << dest << Log::endl;
				to_accept_contexts.push_back("*");
				_service->send__elasticity_contextMigrationQueryReply(dest, to_accept_contexts);
				for( uint32_t i=0; i<to_migrate_contexts.size(); i++ ) {
					this->requestContextMigration( to_migrate_contexts[i], dest );
				}
			}
		} else {  // reject for final agreement
			ASSERT(false);
		}
	} else if( exchangeProcessStep == 1 ) {
		if( star_flag ) { // agree final agreement
			exchangeProcessStep = 2;
			sl.unlock();
			macedbg(1) << "LEM agree the final request from " << dest << Log::endl;
			for( uint32_t i=0; i<to_migrate_contexts.size(); i++ ) {
				this->requestContextMigration( to_migrate_contexts[i], dest );
			}
		} else if( migrate_ctx_flag ) {
			sl.unlock();
			if( contexts.size() > 1 ){
				if( waitingReplyAddr == dest ) {
					macedbg(1) << "The LEM is waiting for a final reply from the same server: " << dest << Log::endl;
					if( Util::getMaceAddr() < dest ) {
						sl.lock();
						exchangeProcessStep = 2;
						sl.unlock();
						macedbg(1) << "LEM agree the final request from " << dest << Log::endl;
						to_accept_contexts.push_back("*");
						_service->send__elasticity_contextMigrationQueryReply(dest, to_accept_contexts);
						for( uint32_t i=0; i<to_migrate_contexts.size(); i++ ) {
							this->requestContextMigration( to_migrate_contexts[i], dest );
						}
					}
				} else {
					macedbg(1) << "The LEM is waiting for a final reply. It rejects second step reply request from " << dest << Log::endl;
					mace::vector<mace::string> reject_ctxes;
					_service->send__elasticity_contextMigrationQueryReply(dest, reject_ctxes);
				}
			}
		} else { 
			exchangeProcessStep = 0;
			sl.unlock();
			processExchangeContextsQuery();
		}
	} else if( exchangeProcessStep == 2 ) {
		if( star_flag ) { // final agreement
			ASSERT(false);
		} else if( migrate_ctx_flag ) {
			sl.unlock();
			if( contexts.size() > 1 ){
				macedbg(1) << "LEM reject reply from " << dest << Log::endl;
				mace::vector<mace::string> reject_ctxes;
				_service->send__elasticity_contextMigrationQueryReply(dest, reject_ctxes);
			}
		}
	}
}

void* mace::eMonitor::startElasticityBehaviorHandlerThread(void* arg) {
	struct ElasticityThreadArg* etarg = ((struct ElasticityThreadArg*)arg);
	eMonitor* monitor = etarg->elasticityMonitor;
	monitor->runElasticityBehaviorHandle();
	return 0;
}

mace::CPUInformation mace::eMonitor::getCurrentCPUInfo() const {
	CPUInformation cpuInfo;

	FILE* file = fopen("/proc/stat", "r");
	unsigned long long int user, low, sys, idle;
    fscanf(file, "cpu %llu %llu %llu %llu", &user, &low, &sys, &idle);
    fclose(file);

    // Time units are in USER_HZ or Jiffies (typically hundredths of a second)
    cpuInfo.totalUserCPUTime = (uint64_t)user;
    cpuInfo.totalUserLowCPUTime = (uint64_t)low;
    cpuInfo.totalSysCPUTime = (uint64_t)sys;
    cpuInfo.totalIdleCPUTime = (uint64_t)idle;

    return cpuInfo;
}

double mace::eMonitor::computeCurrentCPUUsage() const {
	CPUInformation cpuInfo = this->getCurrentCPUInfo();
	double percent = 0.0;

	if ( cpuInfo.totalUserCPUTime < lastcpuInfo.totalUserCPUTime || cpuInfo.totalUserLowCPUTime < lastcpuInfo.totalUserLowCPUTime ||
    		cpuInfo.totalSysCPUTime < lastcpuInfo.totalSysCPUTime || cpuInfo.totalIdleCPUTime < lastcpuInfo.totalIdleCPUTime ){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    } else{
        percent = (cpuInfo.totalUserCPUTime - lastcpuInfo.totalUserCPUTime) + (cpuInfo.totalUserLowCPUTime - lastcpuInfo.totalUserLowCPUTime)
    		+ (cpuInfo.totalSysCPUTime - lastcpuInfo.totalSysCPUTime);
        double total = percent + ( cpuInfo.totalIdleCPUTime - lastcpuInfo.totalIdleCPUTime );
        percent = (percent / total) * 100;
    }

    return percent;
}

uint64_t mace::eMonitor::computeCPUTotalTime() const {
	CPUInformation cpuInfo = this->getCurrentCPUInfo();

	uint64_t total_cpu_time = (cpuInfo.totalUserCPUTime - lastcpuInfo.totalUserCPUTime) + (cpuInfo.totalUserLowCPUTime - lastcpuInfo.totalUserLowCPUTime)
    	+ (cpuInfo.totalSysCPUTime - lastcpuInfo.totalSysCPUTime) + ( cpuInfo.totalIdleCPUTime - lastcpuInfo.totalIdleCPUTime );
    return total_cpu_time;
}

double mace::eMonitor::computeCurrentMemUsage() const {
	struct sysinfo memInfo;

	sysinfo(&memInfo);
	uint64_t totalVirtualMem = memInfo.totalram;
	//Add other values in next statement to avoid int overflow on right hand side...
	totalVirtualMem += memInfo.totalswap;
	totalVirtualMem *= memInfo.mem_unit;

	uint64_t virtualMemUsed = memInfo.totalram - memInfo.freeram;
	//Add other values in next statement to avoid int overflow on right hand side...
	virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
	virtualMemUsed *= memInfo.mem_unit;

	double percent = (virtualMemUsed / totalVirtualMem) * 100;
	return percent;
}

void mace::eMonitor::handleElasticityScaleup( mace::MaceAddr const& idle_server ) {
	mace::set< mace::string > migration_contexts = this->generateMigrationContextSet();

	uint64_t total_migration_contexts_cpu = 0;
	for( mace::set<mace::string>::iterator iter = migration_contexts.begin(); iter != migration_contexts.end(); iter ++ ) {
		total_migration_contexts_cpu += contextRuntimeInfos[ *iter ].getCPUTime();
	}
	// _service->send__elasticity_contextsMigrationRequest( idle_server, migration_contexts, total_migration_contexts_cpu );
}

mace::set<mace::string> mace::eMonitor::generateMigrationContextSet() {
	mace::vector<mace::string> sorted_contexts_by_cpu = sortContextsByCPU();

	mace::set<mace::string> migration_contexts;
	// uint64_t total_migration_contexts_cpu = 0;



	// for( uint32_t i=0; i<sorted_contexts_by_cpu.size(); i++ ) {
	// 	const mace::string& contextName = sorted_contexts_by_cpu[i]; 
	// 	if( migration_contexts.count( contextName ) > 0 ){
	// 		continue;
	// 	}

	// 	migration_contexts.insert( contextName );
	// 	total_migration_contexts_cpu += contextRuntimeInfos[contextName].getCPUTime();

	// 	for( uint32_t j = i; j < sorted_contexts_by_cpu.size(); j ++ ) {
	// 		const mace::string& pairContextName = sorted_contexts_by_cpu[j];
	// 		if( migration_contexts.count(pairContextName) > 0 ) {
	// 			continue;
	// 		}

	// 		if( contextRuntimeInfos[contextName].isStrongConnected(pairContextName) ) {
	// 			migration_contexts.insert( pairContextName );
	// 			total_migration_contexts_cpu += contextRuntimeInfos[pairContextName].getCPUTime();
	// 		}
	// 	}
	// 	if( total_migration_contexts_cpu > mace::eMonitor::MIGRATION_CPU_THRESHOLD ) {
	// 		break;
	// 	}
	// }
	return migration_contexts;
}

mace::vector<mace::string> mace::eMonitor::sortContextsByCPU() const {
	mace::vector<mace::string> empty_vector;
	return empty_vector;
}

void mace::eMonitor::requestContextMigration( const mace::string& contextName, const MaceAddr& destNode ){
  	ADD_SELECTORS("eMonitor::requestContextMigration");
  	macedbg(1) << "Migrating context " << contextName << " to " << destNode <<Log::endl;

  	ContextService* _service = static_cast<ContextService*>(sv);
  	const ContextMapping& snapshot = _service->getLatestContextMapping();

  	mace::MaceAddr addr = mace::ContextMapping::getNodeByContext(snapshot, contextName);
  	if( addr == destNode ) {
  		macedbg(1) << "Context("<< contextName <<") is already on node("<< destNode <<")!" << Log::endl;
  		return;
  	} else if( addr != Util::getMaceAddr() ) {
  		_service->send__migration_contextMigrationRequest( contextName, destNode );
  		return;
  	}
  	uint32_t contextId = mace::ContextMapping::hasContext2( snapshot, contextName );
  	ASSERT( contextId > 0 );

  	ScopedLock sl(migrationMutex);
  	if( contextMigrationRequests.find(contextName) != contextMigrationRequests.end() ){
  		return;
  	} 

  	MigrationRequest request;
  	request.contextName = contextName;
  	request.destNode = destNode;
  	contextMigrationRequests[contextName] = request;

  	migratingContextNames.push_back( contextName );
  	sl.unlock();

  	this->processContextMigration();
}

void mace::eMonitor::processContextMigration(){
	ADD_SELECTORS("eMonitor::processContextMigration");
	ScopedLock sl(migrationMutex);
	if( migratingContextNames.size() > 0 && migratingContextName == "" && toMigrateContext ) {
		mace::string ctx_name = migratingContextNames[0];
		migratingContextName = ctx_name;

		migratingContextNames.erase( migratingContextNames.begin() );
		ASSERT( contextMigrationRequests.find(ctx_name) != contextMigrationRequests.end() );
		const MigrationRequest& request = contextMigrationRequests[ctx_name];
		sl.unlock();

		macedbg(1) << "Start to migrate context("<< request.contextName <<") to " << request.destNode << Log::endl;

		ContextService* _service = static_cast<ContextService*>(sv);
  		const ContextMapping& snapshot = _service->getLatestContextMapping();

  		uint32_t contextId = mace::ContextMapping::hasContext2( snapshot, ctx_name );

  		_service->handleContextMigrationRequest( request.destNode, request.contextName, contextId );
	}
}

void mace::eMonitor::wrapupCurrentContextMigration() {
	ADD_SELECTORS("eMonitor::wrapupCurrentContextMigration");
	ScopedLock sl(migrationMutex);
	ASSERT( migratingContextName != "" );
	macedbg(1) << "Finish context("<< migratingContextName <<") migration!" << Log::endl;
	contextMigrationRequests.erase( migratingContextName );
	migratingContextName = "";
	sl.unlock();

	this->processContextMigration();
}

std::vector<mace::ElasticityBehaviorAction*> mace::eMonitor::generateActionsForElasticityBehavior( ElasticityBehaviorEvent* ebevent ){
	std::vector<mace::ElasticityBehaviorAction*> actions;

	// ContextService* _service = static_cast<ContextService*>(sv);
	// const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	// if( ebevent->eventType == mace::ElasticityBehaviorEvent::EBEVENT_COLOCATE ){
	// 	ElasticityColocateEvent* e = static_cast<ElasticityColocateEvent*>(ebevent->ebhelper);

	// 	const mace::vector<mace::string>& colocateContexts = e->colocateContexts;
	// 	ASSERT( colocateContexts.size() == 2 );

	// 	const mace::MaceAddr addr0 = mace::ContextMapping::getNodeByContext( snapshot, colocateContexts[0] );
	// 	const mace::MaceAddr addr1 = mace::ContextMapping::getNodeByContext( snapshot, colocateContexts[1] );

	// 	ElasticityBehaviorAction* action;

	// 	if( addr0 == Util::getMaceAddr() ){
	// 		double ctx_cpu_usage = _service->getContextCPUUsage(colocateContexts[0]);
	// 	 	action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, ctx_cpu_usage );
	// 		(action->contextNames).push_back( colocateContexts[0] );

	// 		action->destNode = addr1;
	// 	} else if( addr1 == Util::getMaceAddr() ) {
	// 		double ctx_cpu_usage = _service->getContextCPUUsage(colocateContexts[1]);
	// 	 	action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, ctx_cpu_usage );
	// 		(action->contextNames).push_back( colocateContexts[1] );

	// 		action->destNode = addr0;
	// 	} else {
	// 		ASSERT( false );
	// 	}

	// 	actions.push_back( action );
	// } else if( ebevent->eventType == mace::ElasticityBehaviorEvent::EBEVENT_ISOLATE && currentcpuUsage >= eMonitor::CPU_BUSY_THREAHOLD ) {
	// 	ElasticityIsolateEvent* e = static_cast<ElasticityIsolateEvent*>(ebevent->ebhelper);
	// 	mace::map< mace::MaceAddr, double > servers_cpu_usage = _service->getServersCPUUsage();

	// 	double ctx_cpu_usage = _service->getContextCPUUsage(e->contextName);
	// 	for( mace::map< mace::MaceAddr, double>::iterator iter = servers_cpu_usage.begin(); iter != servers_cpu_usage.end(); iter ++ ) {
	// 		if( iter->second + ctx_cpu_usage < eMonitor::CPU_BUSY_THREAHOLD ) {
	// 			ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, 
	// 				ctx_cpu_usage );
	// 			(action->contextNames).push_back( e->contextName );
	// 			action->destNode = iter->first;

	// 			actions.push_back( action );
	// 		}
	// 	}
	// }

	return actions;
}

bool mace::eMonitor::isValidSequentialActions( const std::vector< mace::ElasticityBehaviorAction*>& seq_action) {

	for( uint32_t i=0; i<seq_action.size(); i++ ){
		const mace::ElasticityBehaviorAction* action1 = seq_action[i];
		for( uint32_t j=i+1; j<seq_action.size(); j++ ){
			const mace::ElasticityBehaviorAction* action2 = seq_action[j];
			if( action1->isConfictWith(action2) ){
				return false;
			}
		}
	}

	return true;
}

double mace::eMonitor::computeSeqentialActionScore( const std::vector<ElasticityBehaviorAction*>& seq_action ) {
	double score = 0.0;

	for( uint32_t i=0; i<seq_action.size(); i++ ){
		score += this->computeActionScore(seq_action[i]);
	}

	return score;
}

double mace::eMonitor::computeActionScore( const ElasticityBehaviorAction* action ) {
	// ContextService* _service = static_cast<ContextService*>(sv);
	// const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

	// if( action->actionType == mace::ElasticityBehaviorAction::EBACTION_MIGRATION ){
	// 	const mace::string& contextName = action->getFirstContextName();

	// 	const mace::MaceAddr& local_addr = mace::ContextMapping::getNodeByContext( snapshot, contextName );
	// 	ASSERT( local_addr == Util::getMaceAddr() );

	// 	uint32_t local_conn_strength = _service->getConnectionStrength(0, contextName, local_addr);
	// 	uint32_t remote_conn_strengh = _service->getConnectionStrength(0, contextName, action->destNode);

	// 	double conn_score = this->computeMigrationConnectionScore( local_conn_strength, remote_conn_strengh );

	// 	mace::map< mace::MaceAddr, double > servers_cpu_usage = _service->getServersCPUUsage();

	// 	if( servers_cpu_usage.find(local_addr) == servers_cpu_usage.end() || 
	// 			servers_cpu_usage.find(action->destNode) == servers_cpu_usage.end() ) {
	// 		return 0.0;
	// 	}
	// 	double cpu_score = this->computeMigrationCPUScore( servers_cpu_usage[local_addr], servers_cpu_usage[action->destNode], 
	// 		action->contextCPUUsage);

	// 	return ( conn_score + cpu_score );
	// }

	return 0.0;
}

double mace::eMonitor::computeMigrationConnectionScore( const uint32_t& local_conn_strength, const uint32_t& remote_conn_strengh ) const {
	return ( remote_conn_strengh - local_conn_strength ) / local_conn_strength;
}

double mace::eMonitor::computeMigrationCPUScore( const double& local_server_cpu_usage, const double& remote_server_cpu_usage, 
      const double& ctx_cpu_usage) const {

	double balance_val = std::abs( int( remote_server_cpu_usage+ctx_cpu_usage - local_server_cpu_usage - ctx_cpu_usage ) );
	return balance_val;
}

mace::ElasticityBehaviorAction* mace::eMonitor::getNextAction( std::vector<mace::ElasticityBehaviorAction*>& actions ) {
	if( actions.size() == 0 ){
		return NULL;
	}

	uint64_t largest_benefit = 0;
	std::vector<mace::ElasticityBehaviorAction*>::iterator largest_iter;

	for( std::vector<mace::ElasticityBehaviorAction*>::iterator iter = actions.begin(); iter != actions.end(); iter ++ ){
		if( (*iter)->benefit > largest_benefit ) {
			largest_benefit = (*iter)->benefit;
			largest_iter  = iter;
		}
	}

	if( largest_benefit > 0 ){
		mace::ElasticityBehaviorAction* action = *largest_iter;
		*largest_iter = NULL;
		actions.erase( largest_iter );
		return action;
	} else {
		return NULL;
	}
}

double mace::eMonitor::adjustContextMigrationThreshold( std::vector<mace::ContextBaseClass*>& contexts, const mace::string& marker ) {
	ADD_SELECTORS("eMonitor::adjustContextMigrationThreshold");

	ScopedLock sl(migrationMutex);
	if( migratedContextsInfo.size() < 2 ){
		migratedContextsInfo.clear();
		return 0.0;
	}

	uint32_t total_mc_count = migratedContextsInfo.size();
	uint32_t worse_mc_count = 0;
	double total_diff = 0.0;

	for( uint32_t i=0; i<contexts.size(); i++ ) {
		if( migratedContextsInfo.find(contexts[i]->contextName) != migratedContextsInfo.end() ) {
			double cur_latency = contexts[i]->getMarkerAvgLatency(marker);
			double pre_latency = migratedContextsInfo[ contexts[i]->contextName ].latency;

			macedbg(1) << "context("<< contexts[i]->contextName <<") curLatency=" << cur_latency << ", preLatency=" << pre_latency << Log::endl;

			double diff = cur_latency - pre_latency;
			if( diff > 0 && diff / pre_latency > 0.05 ){
				worse_mc_count += 1;
				total_diff = migratedContextsInfo[ contexts[i]->contextName ].benefitPercent;
			}
		}
	}

	double adjust_val = 0.0;
	if( worse_mc_count/total_mc_count > INCREASE_MIGRATION_THRESHOLD_PERCENT ) {
		adjust_val = total_diff / worse_mc_count;
		macedbg(1) << "adjust_val=" << adjust_val << Log::endl;
	}
	migratedContextsInfo.clear();
	return adjust_val;
}