#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <string>

#include "SysUtil.h"
#include "lib/mace.h"
#include "mlist.h"
#include "RandomUtil.h"
#include "mace-macros.h"
#include <ScopedLock.h>
#include "Event.h"
#include "HierarchicalContextLock.h"

#include "TcpTransport-init.h"
#include "CondorHeartBeat-init.h"
#include "load_protocols.h"

#include "ContextJobNode.h"
//global variables
static bool isClosed = false;

class WorkerJobHandler:public ContextJobNode {
public:
  WorkerJobHandler(){
      std::cout<<"i'm worker"<<std::endl;
      //installSignalHandlers();
      
      params::set("MACE_PORT", boost::lexical_cast<std::string>(30000 + params::get<uint32_t>("pid",0 )*5)  );
      if( params::containsKey("socket") ){
        // TODO: create thread and estbalish domain socket connection
        pthread_create( &commThread, NULL, setupDomainSocket, (void *)this );

      }
  }
  ~WorkerJobHandler(){
      ADD_SELECTORS("WorkerJobHandler::~WorkerJobHandler");
      if( jobpid > 0 ){
          maceout<<"Closing fifo fd"<<Log::endl;
          close(connfd);
          close(sockfd);
          unlink( socketFile );
      }
  }
  virtual void installSignalHandlers(){
      //SysUtil::signal(SIGUSR1, &WorkerJobHandler::snapshotCompleteHandler);
      SysUtil::signal(SIGABRT, &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGHUP,  &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGTERM, &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGINT,  &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGSEGV, &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGCHLD, &WorkerJobHandler::shutdownHandler);
      SysUtil::signal(SIGQUIT, &WorkerJobHandler::shutdownHandler);
      //SysUtil::signal(SIGCONT, &WorkerJobHandler::shutdownHandler);
    // SIGPIPE occurs when the reader of the FIFO/socket is disconnected but this process tries to write to the pipe.
    // the best practice is to ignore the signal so that write() returns an error and handles it locally, instead of installing a global signal handler.
    // http://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
    SysUtil::signal(SIGPIPE, SIG_IGN);
  }
  void ignoreSnapshot( const bool ignore, registration_uid_t rid){ 
     isIgnoreSnapshot = ignore;
     if( ignore == false ){
         std::cout<<"Don't ignore snapshot. Send to master to resume."<<std::endl;
     }else{
         std::cout<<"ignore snapshot from the child process"<<std::endl;
     }
     // chuangw: XXX: it'd be better to tell unit_app process not to take snapshot at all, 
     // rather than let it take and ignore.
  }
  // this upcall should be received by the head node
  void requestMigrateContext( const mace::string& contextID, const MaceKey& destNode, const bool isRoot, registration_uid_t rid){ 
    ADD_SELECTORS("WorkerJobHandler::requestMigrateContext");
    std::ostringstream oss;
    oss<<"migratecontext "<< contextID << " " << destNode << " " << (uint32_t)isRoot;
    uint32_t cmdLen = oss.str().size();
    maceout<<"before write to ."<< Log::endl;
    write(connfd, &cmdLen, sizeof(cmdLen) );
    write(connfd, oss.str().data(), cmdLen);
    maceout<<"write cmdLen = "<< cmdLen <<" finished. "<< Log::endl;
  }
  void requestMigrateNode( const MaceKey& srcNode, const MaceKey& destNode, registration_uid_t rid){ 
    ADD_SELECTORS("WorkerJobHandler::requestMigrateNode");
    std::ostringstream oss;
    oss<<"migratenode "<< srcNode << " " << destNode;
    uint32_t cmdLen = oss.str().size();
    maceout<<"before write to ."<< Log::endl;
    write(connfd, &cmdLen, sizeof(cmdLen) );
    write(connfd, oss.str().data(), cmdLen);
    maceout<<"write cmdLen = "<< cmdLen <<" finished. "<< Log::endl;
  }
  void updateVirtualNodes( const mace::map< uint32_t, mace::MaceAddr >& vnodes, registration_uid_t rid){ 
    ADD_SELECTORS("WorkerJobHandler::updateVirtualNodes");
    std::ostringstream oss;
    mace::string buf;
    mace::serialize( buf, &vnodes );
    oss<<"update_vnode ";
    uint32_t cmdLen = oss.str().size();
    uint32_t bufLen = buf.size();
    maceout<<"before write to ."<< Log::endl;
    write(connfd, &cmdLen, sizeof(cmdLen) );
    write(connfd, oss.str().data(), cmdLen);
    write(connfd, &bufLen, sizeof(bufLen) );
    write(connfd, buf.data(), buf.size());
    maceout<<"write cmdLen = "<< cmdLen <<" finished. "<< Log::endl;
  }


    uint32_t spawnProcess(const mace::string& serviceName, const MaceAddr& vhead, const mace::string& monitorName, const ContextMap& mapping, const mace::string& input, const uint32_t myId, const MaceKey& vNode, registration_uid_t rid){
      ADD_SELECTORS("WorkerJobHandler::spawnProcess");
      createDomainSocket();
      if( (jobpid = fork()) == 0 ){
        /* execute the app */
        mace::map<mace::string, mace::string > args;
        args["-service"] = serviceName;
        args["-monitor"] = monitorName;
        args["-pid"] = params::get<mace::string>("pid","0" );
        args["-lib.ContextJobApplication.launcher_socket"] = mace::string( socketFile );
        if( params::containsKey("logdir") ){
            args["-logdir"] = params::get<mace::string>("logdir");
        }
        char **argv;
        //int ret;
        mapToString(args, &argv);
        /*ret = */execvp("unit_app/unit_app",argv/* argv, env parameter */ );
        releaseArgList( argv, args.size()*2+2 );
        return 0;
      }else if( jobpid != (uint32_t)-1 ){
        /* open domain socket and connect to the app */
        openDomainSocket();
        writeInitialContexts(serviceName, vhead, mapping, vNode);
        //writeResumeSnapshot(snapshot);
        writeInput(input);
        writeDone();
        maceout<<"after writing fifo"<<Log::endl;
        return jobpid;
      }else{  // fork returned -1, indicates a failure
        perror("fork");
        return jobpid;
      }
    }
protected:
    static void shutdownHandler(int signum){
        switch( signum ){
            case SIGABRT: std::cout<<"SIGABRT"<<std::endl;break;
            case SIGHUP:  std::cout<<"SIGHUP"<<std::endl;break;
            case SIGTERM: std::cout<<"SIGTERM"<<std::endl;break;
            case SIGINT:  std::cout<<"SIGINT"<<std::endl;break;
            case SIGCONT: std::cout<<"SIGCONT"<<std::endl;break;
            case SIGSEGV: std::cout<<"SIGSEGV"<<std::endl;break;
            case SIGCHLD: std::cout<<"SIGCHLD"<<std::endl;break;
            case SIGQUIT: std::cout<<"SIGQUIT"<<std::endl;break;
        }
        heartbeatApp->notifySignal(signum);

        if( signum == SIGINT ){ // ctrl+c pressed
            isClosed = true;
        }
        if( signum == SIGTERM){
            if( jobpid > 0 ){
                kill( jobpid, SIGTERM );
            }else{
                std::cout<<"Not running jobs currently. Terminate"<<std::endl;
                isClosed = true;
            }
        }
        if( signum == SIGHUP ){
            isClosed = false;   // ignore SIGHUP. this was the bug from Condor
        }
    }
    /*static void snapshotCompleteHandler(int signum){
        std::cout<<"The job finished snapshot!"<<std::endl;
        if( isIgnoreSnapshot ){
            std::cout<<"ignore snapshot"<<std::endl;
            isClosed = true;
            return;
        }
        // TODO: read from snapshot
        char tmpSnapshot[256];

        char current_dir[256];
        if( getcwd(current_dir,sizeof(current_dir)) == NULL ){
            perror("getcwd() failed to return the current directory name");
        }
        chdir("/tmp");
        //sprintf(tmpSnapshot,"%s", snapshotname.c_str() );
        std::fstream snapshotFile( tmpSnapshot, std::fstream::in );
        if( snapshotFile.is_open() ){
            std::cout<<"file opened successfully for reading"<<std::endl;
        }else{
            std::cout<<"file failed to open for reading"<<std::endl;
        }
        snapshotFile.seekg( 0, std::ios::end);
        // XXX assuming snapshot size < 2 GB = 2^31 bytes
        int fileLen = snapshotFile.tellg();
        snapshotFile.seekg( 0, std::ios::beg);

        char* buf = new char[ fileLen ];
        snapshotFile.read(buf, fileLen);

        snapshotFile.close();
        chdir( current_dir );
        mace::string snapshot( buf, fileLen );
        //std::cout<<"heartbeat process read in "<< snapshot.size() <<" bytes. original snapshot file length="<<fileLen<<std::endl;
        std::cout<<"Ready to transmit snapshot to master!"<<std::endl;
        
        heartbeatApp->reportMigration(snapshot);

        delete buf;

        isClosed = true;
    }*/
// chuangw: To diagnose the output & error message on cloud machines, the following redirect stdout/stderr to a dedicated directory for each process.
// This is unnecessary for Condor nodes, because Condor does this automatically.
    void redirectLog( FILE*& fp_out, FILE*&fp_err ){
        char logfile[1024];
        char logdir[1024];
        sprintf(logdir, "%s", (params::get<mace::string>("logdir") ).c_str());
        struct stat logst;
        if( stat( logdir, &logst ) != 0){
            fprintf(stderr, "log directory %s doesn't exist!\n", logdir);
            exit( EXIT_FAILURE);
        }
        sprintf(logdir, "%s/hb", (params::get<mace::string>("logdir") ).c_str());
        if( stat( logdir, &logst ) != 0 ){ // not exist, create it.
            if( mkdir( logdir, 0755 ) != 0 ){
                fprintf(stderr, "log directory %s can't be created!\n", logdir);
                exit( EXIT_FAILURE);
            }
        }else if( !S_ISDIR(logst.st_mode) ){
            fprintf(stderr, "log directory %s exists but is not directory!\n", logdir);
            exit( EXIT_FAILURE);
        }
        sprintf(logdir, "%s/hb/%d", (params::get<mace::string>("logdir") ).c_str(), getpid());
        if( stat( logdir, &logst ) != 0 ){ // not exist, create it.
            if( mkdir( logdir, 0755 ) != 0 ){
                fprintf(stderr, "log directory %s can't be created!\n", logdir);
                exit( EXIT_FAILURE);
            }
        }
        sprintf(logfile, "%s/hb/%d/out-%d.log", (params::get<mace::string>("logdir") ).c_str(), getpid(), getpid());
        close(1); //stdout
        fp_out = fopen(logfile, "a+");
        if( fp_out == NULL ){
            fprintf(stderr, "can't open log file %s!\n", logfile);
            exit( EXIT_FAILURE);
        }
        if( dup( fileno(fp_out) ) < 0 ){
            fprintf(stderr, "can't redirect stdout to logfile %s", logfile);
            exit( EXIT_FAILURE);
        }
        close(2); //stderr
        sprintf(logfile, "%s/err-%d.log", logdir, getpid());
        fp_err = fopen(logfile, "a+");
        if( fp_err == NULL ){
            fprintf(stdout, "can't open log file %s!\n", logfile);
            exit( EXIT_FAILURE);
        }
        if( dup( fileno(fp_err) ) < 0 ){
            fprintf(stdout, "can't redirect stderr to logfile %s", logfile);
            exit( EXIT_FAILURE);
        }
    }
private:
  // this is called if parameter 'socket' is defined. This means the app (which is a head node) creates the socket file.
  static void* setupDomainSocket(void* obj){
    WorkerJobHandler* thisptr = reinterpret_cast< WorkerJobHandler* >( obj );
    thisptr->createDomainSocket( params::get<std::string>("socket").c_str() );
    thisptr->openDomainSocket();
    thisptr->readRegisterRequest();
    /*writeInitialContexts(serviceName, vhead, mapping, vNode);
    writeResumeSnapshot(snapshot);
    writeInput(input);*/
    thisptr->writeDone();
    pthread_exit(NULL );
    return NULL;
  }
  void createDomainSocket(){
    char sockfile[128];

    sprintf(sockfile, "socket-%d", getpid() );
    createDomainSocket ( sockfile );
  }
  void createDomainSocket(const char* sockfile){
    ADD_SELECTORS("WorkerJobHandler::createDomainSocket");
    int len;
    struct sockaddr_un local;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
    }
    local.sun_family = AF_UNIX;
    sprintf(local.sun_path, "/tmp/%s", sockfile );
    maceout<<"Attempting to connect socket file: "<< local.sun_path<< Log::endl;
    strcpy( socketFile, local.sun_path );
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) == -1) {
      perror("bind");
      exit(1);
    }
  }
  void openDomainSocket(){
    ADD_SELECTORS("WorkerJobHandler::openDomainSocket");
    struct sockaddr_un remote;
    if (listen(sockfd, 5) == -1) {
      perror("listen");
      exit(1);
    }
    int t = sizeof(remote);
    if ((connfd = accept(sockfd, (struct sockaddr *)&remote,(socklen_t*) &t)) == -1) {
      perror("accept");
      exit(1);
    }
    maceout<<"domain socket is connected"<<Log::endl;
  }
  void writeInput(const mace::string& input){
      ScopedLock slock( fifoWriteLock );
      if( input.empty() ) return;
      ADD_SELECTORS("WorkerJobHandler::writeInput");
      std::ostringstream oss;
      oss<<"input";
        
      mace::string buf;
      mace::serialize( buf, &input );

      uint32_t cmdLen = oss.str().size(); 
      uint32_t bufLen = buf.size();
      write(connfd, &cmdLen, sizeof(cmdLen) );
      write(connfd, oss.str().data(), cmdLen );
      write(connfd, &bufLen, sizeof(bufLen) );
      write(connfd, buf.data(), buf.size());
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

    return n;
  }
  void readRegisterRequest(){
    ADD_SELECTORS("ContextJobApplication::readRegisterRequest");
    std::string cmd, data;
    ssize_t readlen = readUDSocket( cmd );
    readlen += readUDSocket( data );

    if( cmd.compare("logical_node") == 0 ){
      mace::MaceAddr headAddr;
      std::string serviceName;
      std::istringstream in( data );
      mace::deserialize( in, &headAddr );
      mace::deserialize( in, &serviceName );


      mace::string headAddrStr = Util::getAddrString( headAddr.local, false );
      params::set("app.launcher.headNode", headAddrStr );
    }else{
      maceerr<<"Unexpected domain socket command from the application : "<< cmd << Log::endl;
    }
  }
    void writeInitialContexts( const mace::string& serviceName, const mace::MaceAddr& vhead, const ContextMap& mapping, const MaceKey& vNode){
      ScopedLock slock( fifoWriteLock );
      ADD_SELECTORS("WorkerJobHandler::writeInitialContexts");
      std::ostringstream oss;
      oss<<"loadcontext";
      maceout<<"Before writing ."<< Log::endl;
        
      mace::string buf;
      mace::serialize( buf, &serviceName );
      mace::serialize( buf, &vhead);
      mace::serialize( buf, &(mapping) );
      mace::serialize( buf, &(vNode) );

      uint32_t cmdLen = oss.str().size(); 
      uint32_t bufLen = buf.size();
      write(connfd, &cmdLen, sizeof(cmdLen) );
      write(connfd, oss.str().data(), cmdLen);
      write(connfd, &bufLen, sizeof(bufLen) );
      write(connfd, buf.data(), buf.size());
      maceout<<"Write  done."<< Log::endl;
    }
    /*void writeResumeSnapshot(const mace::string& snapshot){
      ScopedLock slock( fifoWriteLock );
      if( snapshot.empty() ) return;
      ADD_SELECTORS("WorkerJobHandler::writeResumeSnapshot");
      std::ostringstream oss;
      oss<<"snapshot";
        
      mace::string buf;
      mace::serialize( buf, &snapshot );

      uint32_t cmdLen = oss.str().size(); 
      uint32_t bufLen = buf.size();
      write(connfd, &cmdLen, sizeof(cmdLen) );
      write(connfd, oss.str().data(), cmdLen );
      write(connfd, &bufLen, sizeof(bufLen) );
      write(connfd, buf.data(), buf.size());
    }*/
    void writeDone(){
      ScopedLock slock( fifoWriteLock );
      ADD_SELECTORS("WorkerJobHandler::writeDone");
      std::ostringstream oss;
      oss<<"done";

      uint32_t cmdLen = oss.str().size();
      ssize_t n = write(connfd, &cmdLen, sizeof(cmdLen) );
      if( n == -1 ){ perror("write"); }
      n = write(connfd, oss.str().data(), cmdLen);
      if( n == -1 ){ perror("write"); }
    }
    void releaseArgList( char **argv,int mapsize ){
        for(int argc=0;argc < mapsize; argc++ ){
            delete [] argv[argc];
        }
    }
    void mapToString(mace::map<mace::string, mace::string > &args, char*** _argv){
        ADD_SELECTORS("WorkerJobHandler::mapToSTring");
        char **argv = new char*[ args.size()*2+2 ];
        *_argv =  argv;
        
        int argcount = 0;
        argv[0] = new char[sizeof("unit_app"+1)];
        strcpy( argv[0], "unit_app" );
        argcount++;
        for( mace::map<mace::string, mace::string >::iterator argit = args.begin(); argit != args.end(); argit ++,argcount++ ){
            argv[argcount] = new char[ argit->first.size()+1 ];
            strcpy( argv[argcount], argit->first.c_str() );

            argcount++;
            argv[argcount] = new char[ argit->second.size()+1 ];
            strcpy( argv[argcount], argit->second.c_str() );
        }
        argv[argcount] = NULL;

        maceout<<"argcount="<<argcount<<Log::endl;
        for(int i=0;i< argcount;i++){
            maceout<<"argv["<<i<<"]="<< argv[i] << Log::endl;
        }
    }
protected:
    static uint32_t jobpid;
private:
    int sockfd;
    int connfd;
    char socketFile[108];
    static bool isIgnoreSnapshot;
    //static mace::string snapshotname;
    static pthread_mutex_t fifoWriteLock;
    static pthread_t commThread;
};
uint32_t WorkerJobHandler::jobpid = 0;
bool WorkerJobHandler::isIgnoreSnapshot = false;
//mace::string WorkerJobHandler::snapshotname;
pthread_mutex_t WorkerJobHandler::fifoWriteLock;
pthread_t WorkerJobHandler::commThread;

class CondorNode: public WorkerJobHandler{
public:
  CondorNode(){
    /* chuangw: Condor allows just one file in addition to the executable.
     * Therefore, a workaround is to pack everything needed into a tar ball, 
     * and then unpack it when launcher is executed
     * */
    int n = system("tar xvf everything.tar");
    if( n == -1 ){ perror("system"); }
  }
  virtual void installSignalHandlers(){
    //SysUtil::signal(SIGUSR1, &WorkerJobHandler::snapshotCompleteHandler);
    SysUtil::signal(SIGABRT, &WorkerJobHandler::shutdownHandler);
    SysUtil::signal(SIGHUP,  &WorkerJobHandler::shutdownHandler);
    SysUtil::signal(SIGTERM, &CondorNode::vacateHandler);
    SysUtil::signal(SIGINT,  &WorkerJobHandler::shutdownHandler);
    SysUtil::signal(SIGSEGV, &WorkerJobHandler::shutdownHandler);
    SysUtil::signal(SIGCHLD, &WorkerJobHandler::shutdownHandler);
    SysUtil::signal(SIGQUIT, &WorkerJobHandler::shutdownHandler);
    //SysUtil::signal(SIGCONT, &WorkerJobHandler::shutdownHandler);
    // SIGPIPE occurs when the reader of the FIFO/socket is disconnected but this process tries to write to the pipe.
    // the best practice is to ignore the signal so that write() returns an error and handles it locally, instead of installing a global signal handler.
    // http://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
    SysUtil::signal(SIGPIPE, SIG_IGN);
  }
private:
  static void vacateHandler(int signum){
    if( jobpid > 0 ){
      // when receiving SIGTERM, notify the scheduler.
      heartbeatApp->vacate();
    }else{
      std::cout<<"Receiving SIGTERM, but the launcher is idle. Terminate the launcher process."<<std::endl;
      isClosed = true;
    }
  }
};
class CloudNode: public WorkerJobHandler{
public: 
    CloudNode():fp_out(NULL),fp_err(NULL){
      if(  params::containsKey("logdir") ){
        redirectLog( fp_out, fp_err );
      }
    }

    ~CloudNode(){
        if(fp_out != NULL)
            fclose(fp_out);
        if(fp_err != NULL)
            fclose(fp_err);
    }
private:
  FILE* fp_out, *fp_err;
};
/* chuangw: Amazon EC2 Nodes is similar to PlanetLab to users.
 * The user can install whatever is needed, and has ssh access to the node
 *
 * To some extend, once the ipaddresses of the nodes is known, using it is similar
 * to the local cloud nodes.
 * */
class AmazonEC2Node: public WorkerJobHandler{
public:
    AmazonEC2Node(){
        //ABORT("Amazon EC2 Node is not yet support now");
    }
};


int main(int argc, char* argv[]) {
  ADD_SELECTORS("main");

  mace::Init(argc, argv);
  load_protocols(); // enable service configuration 

  params::addRequired("app.launcher.nodetype", "The type of the launcher - cloud/condor/ec2");

  ContextJobNode* node;
  
  std::string nodetype = params::get<mace::string>("app.launcher.nodetype");
  if( nodetype == "condor" ){
    node = new CondorNode();
  }else if( nodetype == "ec2" ){
    node = new AmazonEC2Node();
  }else if( nodetype == "cloud" ){
    node = new CloudNode();
  }else{
    ABORT("Unrecognized launcher node type: parameter app.launcher.nodetype");
  }
  node->installSignalHandlers();

  params::print(stdout);

  node->start();
  while( isClosed == false ){
      SysUtil::sleepm(100);
  }

  node->stop();
  delete node;
  return EXIT_SUCCESS;
}
