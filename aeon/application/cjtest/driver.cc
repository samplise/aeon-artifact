
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "mstring.h"
#include "MaceKey.h"
#include "ContextMapping.h"
#include "Serializable.h"
void socketServer();
int connfd;
int sockfd;
void createDomainSocket();
void openDomainSocket();
typedef mace::map<MaceAddr, mace::set<mace::string> > ContextMapping;
void writeInitialContexts( const mace::string& serviceName, const mace::MaceAddr& vhead, const ContextMapping& mapping, const MaceKey& vNode);
void writeDone();
char sockname[64];
int main(int argc, char* argv[]){
  int app_pid;
  createDomainSocket();
  if( ( app_pid = fork() ) == 0 ){ 
    const char *app_argv[] = {"cjtest", "-socket", sockname, "--run_test=scheduled", NULL };
    /*int ret;
    ret = */execvp("cjtest",const_cast<char**>(app_argv) );
  }else{
    openDomainSocket();
    mace::string serviceName = "Simple";
    mace::MaceAddr vhead = Util::getMaceAddr("cloud01.cs.purdue.edu:5000");
    mace::MaceAddr n1 = Util::getMaceAddr("cloud02.cs.purdue.edu:6000");
    ContextMapping mapping;
    mapping[ n1].insert("");
    mapping[ n1].insert("R[0]");
    MaceKey vNode( mace::vnode, 1 );
    writeInitialContexts(serviceName, vhead, mapping, vNode);
    writeDone();
    writeDone();
  }
}
void openDomainSocket(){
  struct sockaddr_un remote;
  if (listen(sockfd, 5) == -1) {
    perror("listen");
    //exit(1);
  }
  int t = sizeof(remote);
  if ((connfd = accept(sockfd, (struct sockaddr *)&remote,(socklen_t*) &t)) == -1) {
    perror("accept");
    //exit(1);
  }
  //maceout<<"domain socket is connected"<<Log::endl;
}
void createDomainSocket(){
  int len;
  struct sockaddr_un local;

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    //exit(1);
  }
  local.sun_family = AF_UNIX;
  //sprintf(local.sun_path, "/tmp/cjtest%d.sock", getpid() );
  sprintf(local.sun_path, "/tmp/cjtest.sock" );
  strcpy( sockname, local.sun_path );
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(sockfd, (struct sockaddr *)&local, len) == -1) {
    perror("bind");
    //exit(1);
  }
}
void writeInitialContexts( const mace::string& serviceName, const mace::MaceAddr& vhead, const ContextMapping& mapping, const MaceKey& vNode){
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
  ssize_t n = write(connfd, &cmdLen, sizeof(cmdLen) );
  if( n == -1 ){ perror( "write"); }
  n = write(connfd, oss.str().data(), cmdLen);
  if( n == -1 ){ perror( "write"); }
  n = write(connfd, &bufLen, sizeof(bufLen) );
  if( n == -1 ){ perror( "write"); }
  n = write(connfd, buf.data(), buf.size());
  if( n == -1 ){ perror( "write"); }
  maceout<<"Write  done."<< Log::endl;
}
void writeDone(){
  ADD_SELECTORS("WorkerJobHandler::writeDone");
  std::ostringstream oss;
  oss<<"done";

  uint32_t cmdLen = oss.str().size();
  ssize_t n = write(connfd, &cmdLen, sizeof(cmdLen) );
  if( n == -1 ){
    perror( "write");
  }
  n = write(connfd, oss.str().data(), cmdLen);
  if( n == -1 ){
    perror( "write");
  }
}
