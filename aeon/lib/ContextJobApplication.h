#ifndef CONTEXTJOBAPPLICATION_H
#define CONTEXTJOBAPPLICATION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "ServiceFactory.h"
#include "ContextMapping.h"
#include "ContextService.h"
#include "ThreadStructure.h"
#include "ContextLock.h"
#include "Serializable.h"
#include "SysUtil.h"
#include "mvector.h"
#include "HierarchicalContextLock.h"
#include "boost/format.hpp"
#include "StrUtil.h"
#include <istream>
#include "../services/interfaces/ServiceClass.h"
typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;
typedef mace::vector<mace::string> StringVector;
typedef mace::vector<mace::list<mace::string> > StringListVector;
bool operator==( mace::string const& s1, mace::string const& s2 ){
  if( s1.compare(s2) == 0) return true;
  return false;
}
namespace mace{ 
  void sampleLatency(bool flag);
  double getAverageLatency();
  void finishHeadThread(bool isHead);
  //void loadOwnerships2(const mace::vector< mace::pair<mace::string, mace::string> >& ownerships);

  struct ScheduleItem{
        MaceAddr dest;
        uint8_t service;
        StringVector contexts;
        ScheduleItem(){}
        ScheduleItem( MaceAddr const& dest, uint8_t const& service, StringVector const& contexts ):
          dest(dest), service(service), contexts(contexts){ }
      };
      struct ConditionalMigrationItem{
        uint8_t condition;
        uint8_t cond_operator;
        double threshold;
        MaceAddr dest;
        uint8_t service;
        StringVector contexts;
        ConditionalMigrationItem(){}
        ConditionalMigrationItem( uint8_t condition, uint8_t cond_operator, double threshold, MaceAddr const& dest, uint8_t const& service, StringVector const& contexts ):
          condition( condition ), cond_operator( cond_operator ), threshold( threshold ), dest(dest), service(service), contexts(contexts){ }
        static const uint8_t ON_EVENT_LATENCY = 1;


        static const uint8_t GREATER = 1;
        static const uint8_t EQUAL = 2;
        static const uint8_t LESS = 3;
      };


template<typename Service, typename Handler> class RegisterHandlerTrait{
private:
  Handler *handler;
  Service* maceContextService;
public:
  void registerHandler(Service* maceContextService, Handler *handler){
    this->handler = handler;
    this->maceContextService = maceContextService;
    // after service is created, threads are created, but transport is not initialized.
    if( handler != NULL ){
      maceContextService->registerUniqueHandler( *handler );
    }
  }
  void unregisterHandler(){
      maceContextService->unregisterHandler( *handler, -1 ); // unregister default handler
  }
};
template<typename Service > class RegisterHandlerTrait<Service, void >{
public:
  void registerHandler(Service* maceContextService, void *handler){
    // empty: because NullServiceClass does not use callback handlers.
  }
  void unregisterHandler(){

  }
};

// chuangw: XXX: what would happen if in the middle of reading/writing file, and a signal occurs??
template<class T, class Handler = void> class ContextJobApplication{
public:
  enum NodeType { HeadNode, InternalNode };
  ContextJobApplication(): fp_out(NULL), fp_err(NULL), isResuming(false), nodeType( InternalNode ), hasConsole(false), hasScheduledMigration(false),hasConditionalMigration( false ), isDone( false ) {
    ADD_SELECTORS("ContextJobApplication::(constructor)");
    if( params::containsKey("lib.ContextJobApplication.scheduler_addr") || 
        params::containsKey("lib.ContextJobApplication.launcher_socket") ){ // this app will be managed by the scheduler
      startManaged();
    }else{ // running this app in standalone mode.
      startStandAlone();
    }

    addLog( );
  }
  void startManaged(){
    ADD_SELECTORS("ContextJobApplication::startManaged");
    macedbg(1)<<"Enter startManaged"<< Log::endl;
    schedulerAddress = params::get<std::string>("lib.ContextJobApplication.scheduler_addr", "");
    // TODO: tell if this app is executed by launcher or not
    if( params::containsKey("lib.ContextJobApplication.launcher_socket") ){  // launcher exists
      connectLauncher( params::get<std::string>("lib.ContextJobApplication.launcher_socket") );
    }else{
      std::ostringstream oss;
      oss << "sock." << boost::format("%d") % getpid();
      /*pid_t launcher_pid =*/ createLauncherProcess( oss.str() );
      //connectLauncher( params::get<std::string>oss.str() );
    }

    if( params::containsKey("lib.ContextJobApplication.node_type") &&
      params::get<std::string>("lib.ContextJobApplication.node_type") == "internal" ){
      macedbg(1)<<"Will launch this physical node as an internal node"<<Log::endl;
      setNodeType( InternalNode );
    }else{
      macedbg(1)<<"Will launch this physical node as a new head node"<<Log::endl;
      setNodeType( HeadNode );
    }
  }
  void startStandAlone(){
    ADD_SELECTORS("ContextJobApplication::startStandAlone");

      // up to the application to decide which node is the head, and which are not.
  }
  virtual ~ContextJobApplication(){
    globalExit();
    removeRedirectLog();
  }

  T* getServiceObject(){
    return maceContextService; 
  }

  void setNodeType( enum NodeType type ){
    nodeType = type;
  }

  enum NodeType getNodeType( ){
    return nodeType;
  }

  void connectLauncher(const std::string& socketfile){
    //param::set("socket", socketFile);
    this->socketFileName = socketfile;
    openUDSocket();
  }
  pid_t createLauncherProcess(const std::string& sockfilename){
    this->socketFileName = sockfilename;

    pid_t launcher_pid;
    if( (launcher_pid = fork() ) == 0 ){ // child process
      mace::map< mace::string, mace::string > args;
      // which node type? cloud/condor/ec2?
      args["-socket"] = sockfilename;
      // a parameter to tell launcher it is called by the app.
      char **argv;
      std::string launcher = params::get<std::string>("lib.ContextJobApplication.launcher_path", "./launcher");
      mapToString( args, launcher, &argv );
      int ret = execvp( launcher.c_str(), argv );
      if( ret == -1 ){
        perror("execvp");
        exit( EXIT_FAILURE );
      }
      releaseArgList( argv );
      return 0;
    }else if( launcher_pid != -1 ){ // the original process

    }else{ // fork() failed
        perror("fork");
        exit( EXIT_FAILURE );
    }
    return launcher_pid;
  }
  virtual void startService(const mace::string& service, Handler* handler = NULL){
    ADD_SELECTORS("ContextJobApplication::stratService");
    macedbg(1) << "Try to start service "<< service << Log::endl;
    if( udsockInitConfigDone ){
      installSystemMonitor( );
    }
    mace::ServiceFactory<T>::print(stdout);
    maceContextService = &( mace::ServiceFactory<T>::create(service, true) );

    setServiceName( service );

    //RegisterHandlerTrait<T, Handler> trait;
    trait.registerHandler(maceContextService, handler);
    macedbg(1) << "Invoking maceInit() of "<< service << Log::endl;
    maceContextService->maceInit();
  }
  void createConsole(){
    if( getNodeType() != HeadNode ) return;
  // todo: add a thread that reads keyboard input
      int rc = pthread_create( &shellThread, NULL, ContextJobApplication::commandConsole, (void *)this );
      if( rc != 0 ){
          errno = rc;
          perror("pthread_create() failed");
          exit(EXIT_FAILURE);
      }
      hasConsole = true;
  }
  static void *commandConsole(void *obj){
      ContextJobApplication<T, Handler>* thisptr = reinterpret_cast< ContextJobApplication<T, Handler>* >( obj );
      std::cout<<"For help, type 'help' or '?'"<<std::endl;
      std::cin.exceptions( std::ios::badbit | std::ios::eofbit | std::ios::failbit );
      while(true){
          std::cout<<">>> ";
          std::string cmd;
          try{
              getline(std::cin, cmd);
          }catch( std::ios_base::failure& ex ){
              std::cout<< ex.what() << std::endl;
              break;
          }
          if( cmd.size() == 0 )continue;
          istringstream iss(cmd );
          if( thisptr->executeCommon( iss ) < 0 ){
              break;
          }
      }
      pthread_exit(NULL);
      return NULL;
  }
  int32_t executeCommon(std::istream& iss){
      char cmdbuf[256];
      iss>>cmdbuf;
      if( iss.bad() || iss.fail() ){
          return -1;
      }
      if( strcmp( cmdbuf, "migrate") == 0 ){
          
          iss>>cmdbuf;
          MaceKey destNode(ipv4, cmdbuf);
          uint16_t service;
          iss>>service;
          if( iss.fail() ){
            std::cerr<<"failed to read destination node or service name"<<std::endl;
          }
          iss.getline( cmdbuf, sizeof(cmdbuf) );
          StringList context_names = StrUtil::split(" ", cmdbuf);
          for (StringList::const_iterator i = context_names.begin(); i != context_names.end(); i++) {
            std::cout<<"migrate context "<< *i << " to node "<< destNode << std::endl;
            this->getServiceObject()->requestContextMigration( static_cast<uint8_t>(service), *i, destNode.getMaceAddr(), true );
          }
      }else if( strcmp( cmdbuf,"exit") == 0 ){
          return -1;
      }else{
          std::cerr<<"Unrecognized command: '"<< cmdbuf<<"'"<<std::endl;
      }
      return 0;
  }
  virtual void setTimedMigration(){
    if( !params::containsKey("lib.ContextJobApplication.timed_migrate") &&
    !params::containsKey("lib.ContextJobApplication.timed_migration") 
    ){
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<< "lib.ContextJobApplication.timed_migrate or timed_migration is not set. will not start migration" << std::endl;
      }
      return;
    }
    if( getNodeType() != HeadNode ){
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<< " not head node. will not start migration" << std::endl;
      }
      return;
    }

    // make sure this is the head node
    
    startTimedMigrationThread();
  }
  virtual void setConditionalMigration(){
    if( getNodeType() != HeadNode ){
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<< " not head node. will not start conditional migration" << std::endl;
      }
      return;
    }
    if( !params::containsKey("lib.ContextJobApplication.conditional_migrate") &&
    !params::containsKey("lib.ContextJobApplication.conditional_migration") 
    ){
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<< "lib.ContextJobApplication.conditional_migrate or conditional_migration is not set. will not migration on condition" << std::endl;
      }
      return;
    }

    // make sure this is the head node
    
    startConditionalMigrationThread();
  }
  void startTimedMigrationThread(){

      int rc = pthread_create( &mThread, NULL, ContextJobApplication::runTimedMigrationThread, (void *)this );
      if( rc != 0 ){
          errno = rc;
          perror("pthread_create() failed");
          exit(EXIT_FAILURE);
      }
      hasScheduledMigration = true;
  }
  void startConditionalMigrationThread(){

      int rc = pthread_create( &cThread, NULL, ContextJobApplication::runConditionalMigrationThread, (void *)this );
      if( rc != 0 ){
          errno = rc;
          perror("pthread_create() failed");
          exit(EXIT_FAILURE);
      }
      hasConditionalMigration = true;
  }
  static void* runConditionalMigrationThread(void* obj ){
      ContextJobApplication<T, Handler>* thisptr = reinterpret_cast< ContextJobApplication<T, Handler>* >( obj );
      thisptr->migrateOnCondition();
      return NULL;
  }
  void migrateOnCondition(){
    StringList paramset;
    paramset = StrUtil::split(" ", params::get<std::string>("lib.ContextJobApplication.conditional_migration"));
    std::vector< ConditionalMigrationItem > migrateCondition;
    uint32_t sample_period = 0;
    for (StringList::const_iterator i = paramset.begin(); i != paramset.end(); i++) {
      std::string const& param_id = *i;
      std::cout<<"conditional migrate parameter set id: "<< param_id << std::endl;
      if( !params::containsKey( param_id + ".condition" )  ){
        std::cerr<<"Missing "<< param_id << ".condition parameter"<<std::endl;
        continue;
      }else if(  !params::containsKey( param_id + ".dest" ) ){
        std::cerr<<"Missing "<< param_id << ".dest parameter"<<std::endl;
        continue;
      }else if( !params::containsKey( param_id + ".service" ) ){
        std::cerr<<"Missing "<< param_id << ".service parameter"<<std::endl;
        continue;
      }else if( !params::containsKey( param_id + ".contexts" ) ){
        std::cerr<<"Missing "<< param_id << ".contexts parameter"<<std::endl;
        continue;
      }else if( !params::containsKey( param_id + ".sample_period" ) ){
        std::cerr<<"Missing "<< param_id << ".sample_period parameter"<<std::endl;
        continue;
      }
      StringList condition_param = StrUtil::split(" ", params::get<std::string>( param_id + ".condition" ));
      uint8_t condition;
      uint8_t condition_operator;
      double threshold;
      if( condition_param[0].compare("ON_EVENT_LATENCY") == 0 ){
        sampleLatency(true);
        condition = ConditionalMigrationItem::ON_EVENT_LATENCY;
      }
      if( condition_param[1].compare("GREATER") == 0 ){
        condition_operator = ConditionalMigrationItem::GREATER;
      }else if( condition_param[1].compare("EQUAL") == 0 ){
        condition_operator = ConditionalMigrationItem::EQUAL;
      }else if( condition_param[1].compare("LESS") == 0 ){
        condition_operator = ConditionalMigrationItem::LESS;
      }else{
        std::cerr<<"unrecognized condition operator"<< std::endl;
        continue;
      }

      threshold = boost::lexical_cast<double>( condition_param[2] );

      MaceAddr dest = MaceKey(ipv4, params::get<std::string>( param_id + ".dest" ) ).getMaceAddr();
      uint8_t service = static_cast<uint8_t>(params::get<uint32_t>( param_id + ".service" ));
      StringVector mapping = split(params::get<mace::string>( param_id + ".contexts" ), '\n');
      sample_period = params::get<uint32_t>( param_id + ".sample_period" );
      migrateCondition.push_back( ConditionalMigrationItem( condition, condition_operator, threshold, dest, service, mapping ) );

    }
    if( params::get("lib.ContextJobApplication.debug",false )==true){
      std::cout<< migrateCondition.size() << " conditional migration requests"<< std::endl;
    }
    while(true ){
      // sleep for some time until same_period
      SysUtil::sleep( sample_period );

      // read sampling data
      for( std::vector< ConditionalMigrationItem >::const_iterator mcIt = migrateCondition.begin(); mcIt != migrateCondition.end(); mcIt++ ){
        switch( mcIt->condition ){
          case ConditionalMigrationItem::ON_EVENT_LATENCY:{
            double avgLatency = getAverageLatency();
            if( params::get("lib.ContextJobApplication.debug",false )==true){
              std::cout<<"average event latency = "<< avgLatency << " us"<< std::endl;
            }
            migrateOnLatencyCondition( avgLatency, *mcIt );
          }
        }
      }
    }
  }

  void migrateOnLatencyCondition( double avgLatency, ConditionalMigrationItem const& condition ){
    bool needMigration = false;
    switch( condition.cond_operator ){
      case ConditionalMigrationItem::GREATER:
        if( avgLatency > condition.threshold ) needMigration = true;
        break;
      case ConditionalMigrationItem::LESS:
        if( avgLatency < condition.threshold ) needMigration = true;
        break;
    }
    if( needMigration && params::get("lib.ContextJobApplication.debug",false )==true){
      std::cout<<"average event latency = "<< avgLatency << ", threshold = "<< condition.threshold << ". migrate"<< std::endl;
    }
    if( needMigration ){
      for( StringVector::const_iterator ctxIt = condition.contexts.begin(); ctxIt != condition.contexts.end(); ctxIt ++ ){
        mace::string contextID = *ctxIt;
        getServiceObject()->requestContextMigration( condition.service, contextID, condition.dest , false);
        std::cout << " migrate context "<< contextID <<" of service "<< condition.service <<std::endl;
      }
    }

  }
  static void* runTimedMigrationThread(void* obj ){
      ContextJobApplication<T, Handler>* thisptr = reinterpret_cast< ContextJobApplication<T, Handler>* >( obj );

      std::multimap< uint32_t, ScheduleItem > migrateSchedule;

      StringList paramset;
      if( params::containsKey("lib.ContextJobApplication.timed_migrate") ){
        paramset = StrUtil::split(" ", params::get<std::string>("lib.ContextJobApplication.timed_migrate"));
      }else if( params::containsKey("lib.ContextJobApplication.timed_migration") ) {
        paramset = StrUtil::split(" ", params::get<std::string>("lib.ContextJobApplication.timed_migration"));
      }
      for (StringList::const_iterator i = paramset.begin(); i != paramset.end(); i++) {
        std::string const& param_id = *i;
        std::cout<<"timed migrate parameter set id: "<< param_id << std::endl;
        if( !params::containsKey( param_id + ".time" )  ){
          std::cerr<<"Missing "<< param_id << ".time parameter"<<std::endl;
          continue;
        }else if(  !params::containsKey( param_id + ".dest" ) ){
          std::cerr<<"Missing "<< param_id << ".dest parameter"<<std::endl;
          continue;
        }else if( !params::containsKey( param_id + ".service" ) ){
          std::cerr<<"Missing "<< param_id << ".service parameter"<<std::endl;
          continue;
        }else if( !params::containsKey( param_id + ".contexts" ) ){
          std::cerr<<"Missing "<< param_id << ".contexts parameter"<<std::endl;
          continue;
        }

        uint32_t mTime = params::get<uint32_t>( param_id + ".time" );
        MaceAddr dest = MaceKey(ipv4, params::get<std::string>( param_id + ".dest" ) ).getMaceAddr();
        StringVector mapping = thisptr->split(params::get<mace::string>( param_id + ".contexts" ), ' ');
        uint8_t service = static_cast<uint8_t>(params::get<uint32_t>( param_id + ".service" ));
        //migrateSchedule[ mTime ] = ScheduleItem( dest, service, mapping );
        migrateSchedule.insert( std::pair<uint32_t, ScheduleItem>( mTime , ScheduleItem( dest, service, mapping ) ) );

      }
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<< migrateSchedule.size() << " timed migration requests"<< std::endl;
      }

      uint32_t passedTime = 0;
      for( std::multimap< uint32_t, ScheduleItem >::iterator schedIt = migrateSchedule.begin(); schedIt != migrateSchedule.end(); schedIt++){
        ASSERT( schedIt->first - passedTime >= 0 );
        uint32_t nextTime = schedIt->first - passedTime;
        if( params::get("lib.ContextJobApplication.debug",false )==true){
          std::cout<<"wait for "<< nextTime <<" micro seconds"<<std::endl;
        }
        if( nextTime > 0 ){ // if the timestamp of the next migration request is the same, just do it without calling sleepu()
          SysUtil::sleepu( nextTime );
        }

        if( params::get("lib.ContextJobApplication.debug",false )==true){
          for( StringVector::iterator ctxIt = schedIt->second.contexts.begin(); ctxIt != schedIt->second.contexts.end(); ctxIt ++ ){
            mace::string contextID = *ctxIt;
            std::cout<<"this time, migrate "<< contextID <<" of service "<< schedIt->second.service <<std::endl;
          }
        }
        for( StringVector::iterator ctxIt = schedIt->second.contexts.begin(); ctxIt != schedIt->second.contexts.end(); ctxIt ++ ){
          mace::string contextID = *ctxIt;
          std::cout << " migrate context "<< contextID <<" of service "<< schedIt->second.service <<std::endl;
          thisptr->getServiceObject()->requestContextMigration(schedIt->second.service, contextID, schedIt->second.dest , true);
        }

        passedTime+= nextTime;

        if( params::get("lib.ContextJobApplication.debug",false )==true){
          std::cout<<"passed time: "<< passedTime << std::endl;
        }
      }
      if( params::get("lib.ContextJobApplication.debug",false )==true){
        std::cout<<"migration scheduler thread left..."<< std::endl;
      }
      return NULL;
  }
  virtual void waitService(const uint64_t runtime = 0 ){
    // if this is not head node. don't wait. call maceExit right away.
    if( runtime == 0 ){
      // runtime == 0 means indefinitely.
      while ( !stopped ){
          SysUtil::sleepm(100);
      }
    }else{
        SysUtil::sleepu(runtime);
    }
    //std::cout<<"Prepare to stop"<< std::endl;
    // wait for the console to terminate
    if( hasConsole ){
      void *status;
      int rc = pthread_join(shellThread, &status);
      if( rc != 0 ){
          errno = rc;
          perror("pthread_join() failed");
          exit(EXIT_FAILURE);
      }
    }
    if( hasScheduledMigration ){
      void *status;
      int rc = pthread_join(mThread, &status);
      if( rc != 0 ){
          errno = rc;
          perror("pthread_join() failed");
          exit(EXIT_FAILURE);
      }
    }
    if( hasConditionalMigration ){
      void *status;
      int rc = pthread_join(cThread, &status);
      if( rc != 0 ){
          errno = rc;
          perror("pthread_join() failed");
          exit(EXIT_FAILURE);
      }
    }

  }
  virtual void globalExit(){
    ADD_SELECTORS("ContextJobApplication::globalExit");

    if( isDone ) return;
    
    maceout<<"Prepare to exit..."<<Log::endl;
    maceContextService->maceExit();
    maceout<<"ready to terminate the process"<<Log::endl;
    // after all events have committed, stop threads
    mace::finishHeadThread( (getNodeType()== HeadNode ) );
    ServiceClass::deleteServices();
    mace::Shutdown();
    //trait.unregisterHandler();

    isDone = true;
  }
  /*bool resumeServiceFromFile(mace::Serializable* maceContextService, mace::string serializeFileName ){
      ADD_SELECTORS("resumeServiceFromFile");
      mace::Serializable* serv = dynamic_cast<mace::Serializable*>(maceContextService);

      char resumeFileName[256];
      char current_dir[256];
      if( getcwd(current_dir,sizeof(current_dir)) == NULL ){
          perror("getcwd() failed to return the current directory name");
      }
      chdir("/tmp");
      sprintf(resumeFileName, "%s", serializeFileName.c_str() );
      std::ifstream ifs( resumeFileName, std::ifstream::in );
      mace::deserialize( ifs, serv );

      // TODO: need to consider layered services later. 

      ifs.close();
      chdir( current_dir );
      return true;
  }*/
  void loadContext(){
    ADD_SELECTORS("ContextJobApplication::loadContext");
    macedbg(1) << "Try to load context." << Log::endl;
    if( params::containsKey("lib.ContextJobApplication.services") ){
      if( params::containsKey("lib.ContextJobApplication.nodeset") ){
        loadContextFromParam();
      }else{
        ABORT("lib.ContextJobApplication.nodeset not set");
      }
    }else{
      // by default a physical node is a head node
        setNodeType( HeadNode );
    }
  }
  /* override the default context */
  void loadContext( const mace::map< mace::string, ContextMappingType >& contexts ){
    ADD_SELECTORS("ContextJobApplication::loadcontext");
    macedbg(1) << "load context: " << contexts << Log::endl;
    mace::ContextMapping::setInitialMapping( contexts );
    
    determineNodeType();
  }

  void loadOwnerships( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships) {
    ADD_SELECTORS("ContextJobApplication::loadOwnerships");
    macedbg(1) << "load ownerships: " << ownerships << Log::endl;
    //loadOwnerships2(ownerships);
    ContextStructure::setInitialContextOwnerships(ownerships);
  }
  virtual void installSignalHandler(){
    //SysUtil::signal(SIGTERM, &shutdownHandler); 
    //SysUtil::signal(SIGQUIT, &shutdownHandler); // CTRL+ slash
    //SysUtil::signal(SIGUSR2, &snapshotHandler); // taking snapshot only
    //SysUtil::signal(SIGUSR1, &contextUpdateHandler); // update context
    //SysUtil::signal(SIGCHLD, &childTerminateHandler);
  }
  void addLog(){
    /*if( params::get<bool>("TRACE_ALL",false) == true )
      Log::autoAdd(".*");
    else if( params::containsKey("TRACE_SUBST") ){
      std::istringstream in( params::get<std::string>("TRACE_SUBST") );
      while(in){
        std::string logPattern;
        in >> logPattern;
        if( logPattern.length() == 0 ) break;

        Log::autoAdd(logPattern);
      }
    }
    */
  }
  void redirectLog( const std::string& logdirstr ){
    char logfile[1024];
    char logdir[1024];
    sprintf(logdir, "%s", logdirstr.c_str());
    struct stat logst;
    if( stat( logdir, &logst ) != 0){
        fprintf(stderr, "log directory %s doesn't exist!\n", logdir);
        exit( EXIT_FAILURE);
    }
    sprintf(logdir, "%s/ua", logdirstr.c_str());
    if( stat( logdir, &logst ) != 0 ){ // not exist, create it.
        sprintf(logdir, "%s/ua", logdirstr.c_str());
        if( mkdir( logdir, 0755 ) != 0 ){
            fprintf(stderr, "log directory %s can't be created!\n", logdir);
            exit( EXIT_FAILURE);
        }
    }else if( !S_ISDIR(logst.st_mode) ){
        fprintf(stderr, "log directory %s exists but is not directory!\n", logdir);
        exit( EXIT_FAILURE);
    }
    sprintf(logdir, "%s/ua/%d", logdirstr.c_str(), getpid());
    if( mkdir( logdir, 0755 ) != 0 ){
        //fprintf(stderr, "log directory %s can't be created!\n", logdir);
        //exit( EXIT_FAILURE);
    }
    sprintf(logfile, "%s/out-%d.log", logdir, getpid());
    close(1); //stdout
    fp_out = fopen(logfile, "a+");
    if( dup( fileno(fp_out) ) < 0 ){
        fprintf(stderr, "can't redirect stdout to logfile %s", logfile);
    }
    close(2); //stderr
    sprintf(logfile, "%s/err-%d.log", logdir, getpid());
    fp_err = fopen(logfile, "a+");
    if( dup( fileno(fp_out) ) < 0 ){
        fprintf(stdout, "can't redirect stdout to logfile %s", logfile);
    }
  }
  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
      elems.push_back(item);
    }
    return elems;
  }
  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }
  void loadContextFromParam(){
    mace::map< mace::string, ContextMappingType > contexts;

    NodeSet ns = params::get<NodeSet>("lib.ContextJobApplication.nodeset");
    typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;

    StringList services = StrUtil::split(" ", params::get<std::string>("lib.ContextJobApplication.services"));
    for (StringList::const_iterator i = services.begin(); i != services.end(); i++) {
      ContextMappingType contextMap;
      const std::string service = *i;
      loadServiceContextFromParam( service, ns, contextMap);
      contexts[ service ] = contextMap;
    }

    mace::ContextMapping::setInitialMapping( contexts );
    determineNodeType();
  }
  void loadServiceContextFromParam(std::string const& service, NodeSet& ns, ContextMappingType & contextMap){
    std::ostringstream oss;
    oss<<"lib.ContextJobApplication."<< service << ".mapping";
    std::string map_param = oss.str();
    ASSERT(  params::containsKey( map_param ) );
    StringVector mapping = split(params::get<mace::string>( map_param ), '\n');

    StringListVector node_context;
    ASSERT(ns.size() > 0);
    for( uint32_t i=0; i<ns.size(); i++ ) {
      mace::list<mace::string> string_list;
      node_context.push_back(string_list);
    }
    node_context[0].push_back( mace::ContextMapping::getHeadContext() ); //head context
    node_context[0].push_back( "" ); // global

    for( StringVector::const_iterator it = mapping.begin(); it != mapping.end(); it++ ) {
      StringVector kv = split(*it, ':');
      ASSERT(kv.size() == 2);
      
      uint32_t key;
      istringstream(kv[0]) >> key;
      ASSERT(key >= 0 && key < ns.size());
      node_context[key].push_back(kv[1]);
    }

    int i=0;
    std::vector< MaceAddr > nodeAddrs;
    for( NodeSet::iterator it = ns.begin(); it != ns.end(); it++ ) {
      std::cout << "nodeset[" << i << "] = " << *it << " --> " <<node_context[i] << std::endl;
      contextMap[ (*it).getMaceAddr() ] = node_context[ i++ ];
      nodeAddrs.push_back(  (*it).getMaceAddr() );
    }
  }
  void loadInitContext( mace::string tempFileName ){
      // put temp file into a memory buffer, and then deserialize 
      // the context mapping from the memory buffer.
      char *buf;
      int fileLen = 0;

      std::fstream tempFile( tempFileName.c_str(), std::fstream::in );
      tempFile.seekg( 0, std::ios::end);
      fileLen = tempFile.tellg();
      tempFile.seekg( 0, std::ios::beg);

      buf = new char[ fileLen ];
      tempFile.read(buf, fileLen);
      tempFile.close();
      mace::string servName;
      mace::MaceAddr vhead;

      ContextMappingType mapping;
      mace::string orig_data( buf, fileLen );

      std::istringstream in( orig_data );

      mace::deserialize(in, &servName  );
      mace::deserialize(in, &vhead  );
      mace::deserialize(in, &mapping );

      mapping[ vhead ].push_back( mace::ContextMapping::getHeadContext() );
      mace::map< mace::string, ContextMappingType > servContext;
      servContext[ servName ] = mapping;

      mace::ContextMapping::setInitialMapping( servContext );
      delete buf;
  }
  void loadPrintableInitContext( mace::string& tempFileName ){
      // put temp file into a memory buffer, and then deserialize 
      // the context mapping from the memory buffer.
      ADD_SELECTORS("ContextJobApplication::loadPrintableInitContext");
      char buf[1024];
      mace::map<MaceAddr, mace::list<mace::string> > mapping;
      MaceAddr headnode;
      std::fstream tempFile( tempFileName.c_str(), std::fstream::in );
      mace::string servName;
      while( ! tempFile.eof() ){
          tempFile.getline(buf, 1024);
          if( tempFile.eof() ) break;

          char *tok = strtok( buf, " ");
          if( strncmp( tok, "service:", 8 ) == 0 ){
              char* svname = strtok(NULL, " ");
              servName= svname ;
          } if( strncmp( tok, "head:", 5 ) == 0 ){
              char* node = strtok(NULL, " ");
              const MaceKey headKey( ipv4, node );
              headnode = headKey.getMaceAddr();
          }else{
              MaceAddr mknode = Util::getMaceAddr( tok );
              mace::list<mace::string> contextlist;
              tok = strtok(NULL," ");
              while( tok != NULL ){
                  mace::string context(tok);
                  if( strcmp( tok, "global" ) == 0 ){ 
                      // special case for global context
                      context = "";
                  }
                  contextlist.push_back( context );
                  tok = strtok(NULL, " ");
              }
              mapping[ mknode] = contextlist;

              std::ostringstream oss;
              oss<< mknode<< "is mapped to ";
              for( mace::list<mace::string>::iterator ctxit= contextlist.begin(); ctxit!= contextlist.end(); ctxit++ ){
                  oss<< *ctxit << ",";
              }
              macedbg(1)<< oss.str() << Log::endl;

          }
      }
      tempFile.close();

      mace::map< mace::string, ContextMappingType > servContext;
      mapping[ headnode ].push_back( mace::ContextMapping::getHeadContext() );

      servContext[ servName ] = mapping;

      mace::ContextMapping::setInitialMapping( servContext );
      //mace::ContextMapping::printAll();
  }
  virtual void installSystemMonitor(){
  // TODO: create a thread that monitors the system perofrmance and periodically report to the scheduler.
      pthread_create( &monitorThread, NULL, systemMonitor, (void *)this );
  }
protected:
  void determineNodeType(){
    mace::map < mace::string, ContextMappingType >& contexts = mace::ContextMapping::getInitialMapping( );
    if( params::get("lib.ContextJobApplication.debug",false )==true){
      std::cout<< contexts << std::endl;
    }
    for( mace::map< mace::string, ContextMappingType >::const_iterator it = contexts.begin(); it != contexts.end(); it ++){
      ContextMappingType::const_iterator cit = it->second.find( Util::getMaceAddr() );

      // if this node is not on the initial mapping, it's not the head.
      if( cit == it->second.end() ) {
        if( params::get("lib.ContextJobApplication.debug",false )==true){
          std::cout<<"this node is not on the initial mapping. exit"<<std::endl;
        }
        return;
      }

      // if this node is the head node (based on the initial mapping)
      if( std::find( cit->second.begin(), cit->second.end(),  mace::ContextMapping::getHeadContext() ) != cit->second.end()  ){
        if( params::get("lib.ContextJobApplication.debug",false )==true){
          std::cout<<"this node is set as head"<<std::endl;
        }
        setNodeType( HeadNode );
      }
    }
  }
  static void *systemMonitor(void* obj){
    ContextJobApplication<T, Handler>* thisptr = reinterpret_cast<ContextJobApplication<T, Handler> *>(obj);
    thisptr->realSystemMonitor();
    pthread_exit(NULL);
    return NULL;
  }
  static void *UDSocketComm(void* obj){
    ContextJobApplication<T, Handler>* thisptr = reinterpret_cast<ContextJobApplication<T, Handler> *>(obj);
    thisptr->realUDSocketComm();
    pthread_exit(NULL);
    return NULL;
  }
  void mapToString(mace::map<mace::string, mace::string > &args, std::string& launcher, char*** _argv){
      ADD_SELECTORS("ContextJobApplication::mapToSTring");
      char **argv = new char*[ args.size()*2+2 ];
      *_argv =  argv;
      
      int argcount = 0;
      argv[0] = new char[ launcher.size()+1 ];
      strcpy( argv[0], launcher.c_str() );
      argcount++;
      for( mace::map<mace::string, mace::string >::iterator argit = args.begin(); argit != args.end(); argit ++,argcount++ ){
          argv[argcount] = new char[ argit->first.size()+1 ];
          strcpy( argv[argcount], argit->first.c_str() );

          argcount++;
          argv[argcount] = new char[ argit->second.size()+1 ];
          strcpy( argv[argcount], argit->second.c_str() );
      }
      argv[argcount] = NULL;

      macedbg(1)<<"argcount="<<argcount<<Log::endl;
      for(int i=0;i< argcount;i++){
          macedbg(1)<<"argv["<<i<<"]="<< argv[i] << Log::endl;
      }
  }
  void releaseArgList( char **argv ){
      int argc=0;
      while( argv[argc] != NULL ){
        delete [] argv[argc];
        argc++;
      }
  }
  void realSystemMonitor(){
    // TODO: CPU utilization? 
    // this monitor is only responsible for sending the system performance information. It does not make decision
    // about migration. That is left to the scheduler.
    while( !stopped ){
      // wait some time
      SysUtil::sleep( 60 );
      // gather system performance

      // write UDSocket to notify scheduler
    }
  }
  void realUDSocketComm(){
    ADD_SELECTORS("ContextJobApplication::realUDSocketComm");
    struct sockaddr_un remote;
    int len;
    if ( (sockfd = socket( AF_UNIX, SOCK_STREAM, 0 ) )  == -1 ){
      perror("socket");
      return;
    }
    remote.sun_family = AF_UNIX;
    sprintf( remote.sun_path, "/tmp/%s", socketFileName.c_str() );
    macedbg(1)<<"Attempting to connect to domain socket: "<< remote.sun_path <<Log::endl;
    
    waitForSocketFile( remote.sun_path);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(sockfd, (struct sockaddr *)&remote, len) == -1) {
      perror("connect");
      exit(1);
    }

    if( nodeType == HeadNode ){
      if(  socketFileName.empty() ){ // active head node
        // write new logical node request
        writeRegisterLogicalNodeRequest();
      }else{ // passive head node
        // receive context mapping from scheduler
      }
    }

    macedbg(1)<<"after open read udsock"<<Log::endl;
    readUDSocketInitConfig();
    pthread_mutex_lock(&udsockMutex);
    udsockInitConfigDone = true;
    pthread_cond_signal(&udsockCond );
    pthread_mutex_unlock(&udsockMutex);
    macedbg(1)<<"readUDSocketInitConfig finished"<<Log::endl;
    do{
      std::string udsockdata;
      std::string cmd;
      ssize_t n = readUDCommand(cmd, udsockdata);
      if( n <= 0 ){
        macedbg(1)<<"readUDCommand() returned zero(eof). leaving udsock thread"<<Log::endl;
        break;
      }
      if( cmd.compare("done") == 0 ){
        macedbg(1)<<"received 'done'. leaving"<<Log::endl;
        break;
      }
      istringstream iss( udsockdata );
      if( cmd.compare("migratecontext") == 0 ){
          uint8_t serviceID;
          mace::string contextID;
          uint32_t isRoot;
          mace::string destKeyStr;
          iss>>serviceID;
          iss>>contextID;
          iss>>destKeyStr;
          MaceKey destNode(destKeyStr);
          iss>>isRoot;
          macedbg(1)<< "serviceID="<< serviceID <<", contextID="<<contextID<<", destNode="<< destNode <<", isRoot="<<isRoot<<Log::endl;
          //BaseMaceService* serv = dynamic_cast<BaseMaceService*>(maceContextService);
          //serv->requestContextMigration(serviceID, contextID, destNode.getMaceAddr() , isRoot);
          maceContextService->requestContextMigration(serviceID, contextID, destNode.getMaceAddr() , isRoot);
      }else if( cmd.compare("update_vnode") == 0 ){
          typedef mace::map<uint32_t, MaceAddr > VNodeMappingType;
          VNodeMappingType vnodes;

          mace::deserialize(iss, &vnodes );

          for( VNodeMappingType::iterator vnit = vnodes.begin(); vnit != vnodes.end(); vnit++ ){
              mace::ContextMapping::updateVirtualNodes( vnit->first, vnit->second );
          }
      }else if( cmd.compare("vacate") == 0 ){
        // TODO migrate all contexts
      }else{
          macedbg(1)<<"Unrecognized command '"<<cmd<<"' from launcher process"<<Log::endl;
      }
    // FIXME: how to stop?
    }while(1);

    macedbg(1)<<"UDSocket thread exiting..."<<Log::endl;

    close(sockfd);
    macedbg(1)<<"udsock sockfd closed in UDSocket thread"<<Log::endl;
    return;
  }
  void removeRedirectLog(){
    if( params::containsKey("logdir") ){
      if(fp_out != NULL)
          fclose(fp_out);
      if(fp_err != NULL)
          fclose(fp_err);
    }
  }
  static void shutdownHandler(int signum){
      ADD_SELECTORS("ContextService::shutdownHandler");
      maceout<<"received SIGTERM or SIGINT! Ready to stop."<<Log::endl;

      /**
       * Memo on context migration implementation (chuangw)
       *
       * context migration starts by head. Head somehow detects migrating some contexts
       * is better. Head creates a context migration event to this physical node.
       *
       * When all previous write events to the context are finished, start taking snapshot of the context.
       * and block later events.
       *
       * */

      /**
       * Memo on node migration implementation (chuangw)
       *
       * When a SIGTERM is received, send a message to head to create an event.
       * When event is created and returned to this node, 
       * this node, after all previous write events are finished,
       * start taking snapshot. 
       *
       * 
       * */
      snapshotHandler(signum);

      //if( params::get<bool>("killparent",false) == true ){
        //std::cout<<" Tell launcher process the snapshot is finished"<< std::endl;
        //maceout<<" Tell launcher process the snapshot is finished"<< Log::endl;
        //kill( getppid() , SIGUSR1);
      //}

      maceout << "Exiting at time " << TimeUtil::timeu() << Log::endl;
      
      exit(EXIT_SUCCESS);

      // program will not reach here.
      // All done.
      //
      // chuangw: don't need to gracefully leave. all later events are blocked and we don't want to process them at all.
      // we're being terminate/migrated, why care about system resources being occupied? Not my business.
      maceContextService->maceExit();
      mace::Shutdown();
      if( params::containsKey("pid") ){
        void *status;
        pthread_cancel( commThread );
        pthread_join(commThread, &status);

        pthread_cancel( monitorThread );
        pthread_join( monitorThread, &status);

      }

      stopped = true;
    
  }
  static void snapshotHandler(int signum){
      ADD_SELECTORS("snapshotHandler");
      // chuangw: obsoleted code...... need to make a big change here.
      /*
      // get new ticket, but don't use it. Effectively block all later events
      uint64_t myTicketNum = ThreadStructure::newTicket(true);
      // chuangw: TODO: ThreadStructure::newTicket() does not block.
      // but ContextLock::_context_ticketbooth will block, 
      //
      // after blocking, only the return value of the previous sync call event   and migration event can pass through
      // 
      // migration event need to wait for all previous sync call events to return.
      
      std::cout<<"migration/snapshot ticket="<<myTicketNum<<std::endl;
      maceout<<"migration/snapshot ticket="<<myTicketNum<<Log::endl;
      while( true ){
          // check if all previous events committed every 1 millisecond.
          // XXX: chuangw: I know it's not the good way of doing it...but I'll just do it for the sake of simplicity.
          // it doesn't make difference to wait for one more millisecond anyway.
          SysUtil::sleepu(1000); 
          if( mace::ContextLock::lastCommittedTicket == myTicketNum - 1 ){
              break;
          }
      }
      */
      // if the thread reaches here, all previous events have been committed, and all late events are blocked.
      // we can safely take snapshot

      /*
      mace::Serializable* serv = dynamic_cast<mace::Serializable*>(maceContextService);
      if( serv == NULL ){ // failed to dynamically cast to Serializable. abort
          std::cerr<<"Failed to dynamically cast to Serializable. Abort"<<std::endl;
          maceerr<<"Failed to dynamically cast to Serializable. Abort"<<Log::endl;
          abort();
      }
      mace::string buf;
      mace::serialize( buf, serv );
      // chuangw: XXX: Do I need to keep all the old snapshot??
      std::cout<<"size of snapshot : "<< buf.size() <<std::endl;
      maceout<<"size of snapshot : "<< buf.size() <<Log::endl;

      char current_dir[256];
      if( getcwd(current_dir,sizeof(current_dir)) == NULL ){
          perror("getcwd() failed to return the current directory name");
      }
      if( chdir("/tmp") == -1 ){
          char errormsg[256];
          sprintf(errormsg, "main():%s:%d: can't chdir to /tmp", __FILE__, __LINE__ );
          perror(errormsg);
          abort();
      }
      mace::string snapshotFileName = params::get<mace::string>("snapshot");
      std::cout<<"snapshot file name: "<<snapshotFileName.c_str()<<std::endl;
      maceout<<"snapshot file name: "<<snapshotFileName.c_str()<<Log::endl;
      std::fstream ofs( snapshotFileName.c_str(), std::fstream::out );

      ofs.write( buf.data(), buf.size() );

      ofs.seekg( 0, std::ios::end);
      int fileLen = ofs.tellg();
      maceout<<"[unit_app] the snapshot file len = "<< fileLen<< Log::endl;
      ofs.close();

      chdir( current_dir );
      */
  }
  /**
   * XXX: chuangw: Handling signals in multi-thread process in Linux can be different from other Unix systems.
   * The following code assumes the main thread receives the signal. If child threads receives the signal, it can
   * be much more complicated.....
   * */

  static void childTerminateHandler(int signum){
      // received SIGCHLD
      ADD_SELECTORS("childTerminateHandler");
      pid_t pid;
      int status;
      while( (pid=waitpid(-1,&status, WNOHANG) ) > 0 );
  }

private:
  void waitForSocketFile(const char* sockfile){// periodically check the existence of the specified socket file
    ADD_SELECTORS("ContextJobApplication::waitForSocketFile");
    do {
      struct stat filestat;
      int ret = stat( sockfile , &filestat );
      if( ret == -1 ){
        if( errno == ENOENT ){
          macedbg(1)<<"File not exist. Sleep for 100ms and re-check"<<Log::endl;
          SysUtil::sleepm(100); 
        }else{
          perror( "stat" );
        }
      }else{
        SysUtil::sleepm(100);  // sleep for another 100ms before attempt t connect the domain socket
        break;
      }
    }while (true);
  }
  void openUDSocket(){
    //if( params::containsKey("socket") ){
    pthread_create( &commThread, NULL, UDSocketComm, (void *)this );
    pthread_mutex_lock(&udsockMutex);
    if( udsockInitConfigDone == false ){ // TODO: use pthread_barrier instead
      pthread_cond_wait(&udsockCond, &udsockMutex  );
    }
    // TODO: depending on param ContextJobApplication:node_type,
    // if its 'headnode'
    //    register a new logical node with launcher
    // else if 'internalnode'
    //    load contextmappings from launcher
    //
    pthread_mutex_unlock(&udsockMutex);
    //}
  }
  ssize_t readUDSocket(std::string& str){
    ADD_SELECTORS("ContextJobApplication::readUDSocket");
    uint32_t cmdLen;
    macedbg(1)<<"before read UDSocket"<<Log::endl;
    struct timeval tv;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET( sockfd, &rfds );
    do{
      tv.tv_sec = 10; tv.tv_usec = 0;
      select( sockfd+1, &rfds, NULL, NULL, &tv );

      if( FD_ISSET( sockfd, &rfds ) ){
        macedbg(1)<<"select() has data"<<Log::endl;
        break;
      }else{
        macedbg(1)<<"select() timeout."<<Log::endl;
      }
    }while( true );
    ssize_t n = read(sockfd, &cmdLen, sizeof(cmdLen) );
    macedbg(1)<<"after read cmdLen, before read command"<<Log::endl;
    if( n == -1 ){
      perror("read");
      return n;
    }else if( n < (ssize_t)sizeof(cmdLen) ){ // reaches end of udsock: writer closes the udsock
      maceerr<<"returned string length "<< n <<" is less than expected length "<< sizeof(cmdLen) << Log::endl;
      return n;
    }
    
    char *udsockbuf = new char[ cmdLen ];
    n = read(sockfd, udsockbuf, cmdLen );
    macedbg(1)<<"after read command. read len = "<<n<<Log::endl;
    if( n == -1 ){
      perror("read");
      return n;
    }else if( n < (int)cmdLen ){ // reaches end of udsock: writer closes the udsock
      maceerr<<"returned string length "<< n <<" is less than expected length "<< cmdLen << Log::endl;
      return n;
    }
    str.assign( udsockbuf, cmdLen );
    delete udsockbuf;

    /*if( str.compare( "done" ) == 0 ){
      return n;
    }*/

    return n;
  }
  ssize_t readUDCommand(std::string& udsockcmd, std::string& udsockstr){
    ssize_t readlen = readUDSocket( udsockcmd );
    if( udsockcmd == "done" ){
      return readlen;
    }
    readlen += readUDSocket( udsockstr );

    return readlen;
  }
  void writeRegisterLogicalNodeRequest(){
    ADD_SELECTORS("ContextJobApplication::writeRegisterLogicalNodeRequest");

    mace::MaceAddr headAddr = Util::getMaceAddr();
    mace::string cmdstr = "logical_node";
    mace::string datastr;
    mace::serialize( datastr, &headAddr );
    mace::serialize( datastr, &serviceName );

    uint32_t cmdLen = cmdstr.size();
    uint32_t dataLen = datastr.size();
    int ret = write(sockfd, &cmdLen, sizeof(cmdLen) );
    ret = write(sockfd, cmdstr.data(), cmdLen);
    ret = write(sockfd, &dataLen, sizeof(dataLen) );
    ret = write(sockfd, datastr.data(), dataLen);
    ASSERT(ret != 1);
  }
  void readUDSocketInitConfig(){
    ADD_SELECTORS("ContextJobApplication::readUDSocketInitConfig");
    bool isDone = false;
    do{
      std::string cmd, udsockdata;
      readUDCommand(cmd, udsockdata);

      istringstream iss( udsockdata );
      macedbg(1)<<"UDSocket command: "<< cmd<<Log::endl;
      if( cmd.compare("loadcontext") == 0 ){
        readUDSocketInitialContexts(iss);
      }else if( cmd.compare("snapshot") == 0 ){
        //readUDSocketResumeSnapshot(iss);
      }else if( cmd.compare("done") == 0 ){
        isDone = true;
      }else if( cmd.compare("input") == 0 ){
        readUDSocketInput( iss );
      }/*else if( cmd.compare("logical_node") == 0 ){
        readUDSocketLogicalNodeID( iss );
      }*/else{
          macedbg(1)<<"Unrecognized command '"<<cmd<<"' from launcher process"<<Log::endl;
      }
    }while( !isDone );
  }
  void readUDSocketInitialContexts(std::istringstream& iss){
    ADD_SELECTORS("ContextJobApplication::readUDSocketInitialContexts");
    macedbg(1)<<"loading initial context mapping"<<Log::endl;
    mace::string serviceName;
    mace::MaceAddr vhead;
    ContextMappingType mapping;
    MaceKey vNode;

    mace::deserialize( iss, &serviceName );
    macedbg(1)<<"service name: "<< serviceName<< Log::endl;
    mace::deserialize( iss, &vhead);
    macedbg(1)<<"vhead : "<< vhead << Log::endl;
    mace::deserialize( iss, &(mapping) );
    macedbg(1)<<"mapping: "<< mapping<< Log::endl;
    mace::deserialize(iss, &vNode );
    macedbg(1)<<"vNode: "<< vNode<< Log::endl;
    mace::ContextMapping::setVirtualNodeMaceKey( vNode );

    mapping[ vhead ].push_back( mace::ContextMapping::getHeadContext() );

    mace::map< mace::string, ContextMappingType > servContext;
    servContext[ serviceName ] = mapping;

    mace::ContextMapping::setInitialMapping( servContext );
    macedbg(1)<<"Initial context mapping loaded"<< Log::endl;
  }
  /* FIXME: this function should be called after the service object is created but before maceResume() is called */
  void readUDSocketResumeSnapshot(std::istringstream& iss){
    mace::Serializable* serv = dynamic_cast<mace::Serializable*>(maceContextService);
    mace::deserialize( iss, serv );
    isResuming = true;
  }
  void readUDSocketInput(std::istringstream& iss){
    mace::string input;
    mace::deserialize( iss, &input );
    params::set("input", input );
  }
  void setServiceName( const std::string& service ){
    serviceName = service;
  }
  FILE* fp_out, *fp_err;
  bool  isResuming;
  std::string socketFileName;
  std::string serviceName;

  static T* maceContextService;
  static bool stopped;
  static int sockfd;
  static pthread_t commThread;
  static pthread_t monitorThread;
  static pthread_mutex_t udsockMutex;
  static pthread_cond_t udsockCond;
  static bool udsockInitConfigDone;

  NodeType nodeType;
  std::string schedulerAddress;
  pthread_t shellThread;
  pthread_t mThread;
  pthread_t cThread;
  bool hasConsole;
  bool hasScheduledMigration;
  bool hasConditionalMigration;
  bool isDone;
  RegisterHandlerTrait<T, Handler> trait;
};
template<class T, class Handler> bool mace::ContextJobApplication<T, Handler>::stopped = false;
template<class T, class Handler> T* mace::ContextJobApplication<T, Handler>::maceContextService = NULL;
template<class T, class Handler> pthread_t mace::ContextJobApplication<T, Handler>::commThread;
template<class T, class Handler> pthread_t mace::ContextJobApplication<T, Handler>::monitorThread;
template<class T, class Handler> int mace::ContextJobApplication<T, Handler>::sockfd;
template<class T, class Handler> pthread_mutex_t mace::ContextJobApplication<T, Handler>::udsockMutex;
template<class T, class Handler> pthread_cond_t mace::ContextJobApplication<T, Handler>::udsockCond;
template<class T, class Handler> bool mace::ContextJobApplication<T, Handler>::udsockInitConfigDone;
} // end of mace:: namespace
#endif
