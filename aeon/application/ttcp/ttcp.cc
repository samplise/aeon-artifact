/* 
 * ttcp.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Charles Killian
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
/*
 *	T T C P . C
 *
 * Test TCP connection.  Makes a connection on port 5001
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Usable on 4.2, 4.3, and 4.1a systems by defining one of
 * BSD42 BSD43 (BSD41a)
 * Machines using System V with BSD sockets should define SYSV.
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 * Modified in 1989 at Silicon Graphics, Inc.
 *	catch SIGPIPE to be able to print stats when receiver has died 
 *	for tcp, don't look for sentinel during reads to allow small transfers
 *	increased default buffer size to 8K, nbuf to 2K to transfer 16MB
 *	moved default port to 5001, beyond IPPORT_USERRESERVED
 *	make sinkmode default because it is more popular, 
 *		-s now means don't sink/source 
 *	count number of read/write system calls to see effects of 
 *		blocking from full socket buffers
 *	for tcp, -D option turns off buffered writes (sets TCP_NODELAY sockopt)
 *	buffer alignment options, -A and -O
 *	print stats in a format that's a bit easier to use with grep & awk
 *	for SYSV, mimic BSD routines to use most of the existing timing code
 * Modified by Steve Miller of the University of Maryland, College Park
 *	-b sets the socket buffer size (SO_SNDBUF/SO_RCVBUF)
 * Modified Sept. 1989 at Silicon Graphics, Inc.
 *	restored -s sense at request of tcs@brl
 * Modified Oct. 1991 at Silicon Graphics, Inc.
 *	use getopt(3) for option processing, add -f and -T options.
 *	SGI IRIX 3.3 and 4.0 releases don't need #define SYSV.
 *
 * Distribution Status -
 *      Public Domain.  Distribution Unlimited.
 */
// #ifndef lint
// static char RCSid[] = "ttcp.c $Revision: 1.1 $";
// #endif

#define BSD43
/* #define BSD42 */
/* #define BSD41a */
/* #define SYSV */	/* required on SGI IRIX releases before 3.3 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
/* #include <machine/endian.h> */
#include <sys/types.h>
#include <stdint.h>
#include "m_net.h"
#include <sys/time.h>		/* struct timeval */
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <iostream>

#if defined(SYSV)
#include <sys/times.h>
#include <sys/param.h>
struct rusage {
    struct timeval ru_utime, ru_stime;
};
#define RUSAGE_SELF 0
#else
#include <sys/resource.h>
#endif

#include "params.h"
#include "TcpTransport.h"
#include "TcpTransport-init.h"
#include "TimeUtil.h"
#include "SysUtil.h"

using namespace std;

struct sockaddr_in sinme;
struct sockaddr_in sinhim;
struct sockaddr_in frominet;

int domain;
socklen_t fromlen;
int fd;				/* fd of network socket */

int buflen = 8 * 1024;		/* length of buffer */
char *buf;			/* ptr to dynamic buffer */
int nbuf = 2 * 1024;		/* number of buffers to send in sinkmode */

int bufoffset = 0;		/* align buffer to this */
int bufalign = 16*1024;		/* modulo this */

int udp = 0;			/* 0 = tcp, !0 = udp */
int options = 0;		/* socket options */
int one = 1;                    /* for 4.3 BSD style setsockopt() */
short port = 5377;		/* TCP port number */
char *host;			/* ptr to name of host */
int trans;			/* 0=receive, !0=transmit mode */
int sinkmode = 0;		/* 0=normal I/O, !0=sink/source mode */
int verbose = 0;		/* 0=print basic info, 1=print cpu rate, proc
				 * resource usage. */
int nodelay = 0;		/* set TCP_NODELAY socket option */
int b_flag = 0;			/* use mread() */
int sockbufsize = 0;		/* socket buffer size to use */
char fmt = 'K';			/* output format: k = kilobits, K = kilobytes,
				 *  m = megabits, M = megabytes, 
				 *  g = gigabits, G = gigabytes */
int touchdata = 0;		/* access data after reading */

bool onetflag = false;
mace::string onetbuf;
// on_addr local;
// on_addr dest;
bool maceflag = false;
bool sleepflag = false;
bool macedone = false;
bool macelog = false;
MaceKey macedest;
MaceKey maceforward;
mace::string macelocal;
std::string onetgateway;
BufferedTransportServiceClass* transport;
uint64_t connectStart = 0;
uint64_t connectEnd = 0;
uint64_t openStart = 0;
uint64_t openEnd = 0;
uint64_t localOpenStart = 0;
uint64_t localOpenEnd = 0;
uint64_t initStart = 0;
uint64_t initEnd = 0;
uint64_t configStart = 0;
uint64_t configEnd = 0;
uint64_t sendTime = 0;

struct hostent *addr;
extern int errno;
extern int optind;
extern char *optarg;

const char Usage[] = "\
Usage: ttcp -t [-options] host [ < in ]\n\
       ttcp -r [-options > out]\n\
Common options:\n\
	-l ##	length of bufs read from or written to network (default 8192)\n\
	-u	use UDP instead of TCP\n\
	-p ##	port number to send to or listen at (default 5001)\n\
	-s	-t: source a pattern to network\n\
		-r: sink (discard) all data from network\n\
	-A	align the start of buffers to this modulus (default 16384)\n\
	-O	start buffers at this offset from the modulus (default 0)\n\
	-v	verbose: print more statistics\n\
	-d	set SO_DEBUG socket option\n\
	-b ##	set socket buffer size (if supported)\n\
	-f X	format for rate: k,K = kilo{bit,byte}; m,M = mega; g,G = giga\n\
Options specific to -t:\n\
	-n##	number of source bufs written to network (default 2048)\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY socket option)\n\
Options specific to -r:\n\
	-B	for -s, only output full blocks as specified by -l (for TAR)\n\
	-T	\"touch\": access each byte as it's read\n\
Benchmark options:\n\
        -o X    use ONet; X is the address for your local FIFO\n\
        -g X    use ONet gateway X\n\
        -m X    use Mace TcpTransport; X is your MACE_LOCAL_ADDRESS\n\
        -F X    forward packets to X\n\
        -S      sleep after sending data; do not exit on error\n\
        -L      enable all mace logging\n\
";	

char stats[128];
uint64_t nbytes = 0;		/* bytes on net */
unsigned long numCalls = 0;		/* # of I/O system calls */
double cput, realt;		/* user, real time (seconds) */

void printUsage();
void err(const char* s);
void mes(const char* s);
void pattern(char* cp, int cnt);
void prep_timer();
double read_timer(char *str, int len);
int Nread(int fd, char* buf, int count);
int Nwrite(int fd, void* buf, int count);
void delay(int us);
int mread(int fd, char* bufp, unsigned n);
char *outfmt(double b);

class TransportCallback : public ReceiveDataHandler, public NetworkErrorHandler {
public:
  void error(const MaceKey& id, TransportError::type code, const string& message,
	     registration_uid_t rid) {
    cout << "received error " << message << endl;
    if (!sleepflag) {
      macedone = true;
    }
  }

  void deliver(const MaceKey& source, const MaceKey& destination,
	       const std::string& s, registration_uid_t rid) {
    if (numCalls == 0) {
      prep_timer();
    }
    numCalls++;
    nbytes += s.size();
    if (!maceforward.isNullAddress()) {
      transport->send(maceforward, s);
    }
//     cout << "read " << s.size() << " " << numCalls << endl;
  }
  
}; // TransportCallback

TransportCallback tcb;

void
sigpipe(int)
{
}

int main(int argc, char** argv)
{
  unsigned long addr_tmp;
  int c;

  if (argc < 2) {
    printUsage();
  }

  std::string onetdest;

  while ((c = getopt(argc, argv, "drstuvBDTSLb:f:F:l:n:p:A:O:o:m:g:")) != -1) {
    switch (c) {

    case 'g':
      onetgateway = optarg;
      break;
    case 'S':
      sleepflag = true;
      break;
    case 'L':
      macelog = true;
      break;
    case 'm':
      maceflag = true;
      macelocal = optarg;
      break;
    case 'F':
      maceforward = MaceKey(ipv4, optarg);
      break;
    case 'o':
      onetflag = true;
      onetdest = optarg;
      break;
    case 'B':
      b_flag = 1;
      break;
    case 't':
      trans = 1;
      break;
    case 'r':
      trans = 0;
      break;
    case 'd':
      options |= SO_DEBUG;
      break;
    case 'D':
#ifdef TCP_NODELAY
      nodelay = 1;
#else
      fprintf(stderr, 
	      "ttcp: -D option ignored: TCP_NODELAY socket option not supported\n");
#endif
      break;
    case 'n':
      nbuf = atoi(optarg);
      break;
    case 'l':
      buflen = atoi(optarg);
      break;
    case 's':
      sinkmode = !sinkmode;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'u':
      udp = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'A':
      bufalign = atoi(optarg);
      break;
    case 'O':
      bufoffset = atoi(optarg);
      break;
    case 'b':
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
      sockbufsize = atoi(optarg);
#else
      fprintf(stderr, 
	      "ttcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
#endif
      break;
    case 'f':
      fmt = *optarg;
      break;
    case 'T':
      touchdata = 1;
      break;

    default:
      printUsage();
    }
  }

  if (maceflag) {
    params::set("MACE_LOCAL_ADDRESS", macelocal);
    params::set("MACE_BIND_LOCAL_ADDRESS", "1");
    params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
    params::set("MACE_WARN_LOOPBACK", "0");
    if (macelog) {
      Log::autoAddAll();
    }

    transport = &TcpTransport_namespace::new_TcpTransport_BufferedTransport();
    transport->maceInit();
    transport->registerUniqueHandler((ReceiveDataHandler&)tcb);
    transport->registerUniqueHandler((NetworkErrorHandler&)tcb);
  }

  connectStart = TimeUtil::timeu();

  if(trans)  {
    if (optind == argc) {
      printUsage();
    }
    host = argv[optind];

    /* xmitr */
//     if (onetflag) {
//       local = on_addr(onet::FIFO, "ttcplocal" + StrUtil::toString(getpid()));
//       dest = on_addr(onet::FIFO, host);
//     }
//     else
    if (maceflag) {
      macedest = MaceKey(ipv4, host);
    }
    else {
      bzero((char *)&sinhim, sizeof(sinhim));
      if (atoi(host) > 0 )  {
	/* Numeric */
	sinhim.sin_family = AF_INET;
	sinhim.sin_addr.s_addr = inet_addr(host);
      } else {
	if ((addr=gethostbyname(host)) == NULL)
	  err("bad hostname");
	sinhim.sin_family = addr->h_addrtype;
	bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
	sinhim.sin_addr.s_addr = addr_tmp;
      }
      sinhim.sin_port = htons(port);
      sinme.sin_port = 0;		/* free choice */
    }
  } else {
    /* rcvr */
//     if (onetflag) {
//       local = on_addr(onet::FIFO, onetdest);
//       local.wait(true);
//     }
//     else
    if (!maceflag) {
      sinme.sin_port =  htons(port);
    }
  }

  if (udp && buflen < 5) {
    buflen = 5;		/* send more than the sentinel size */
  }

  if ( (buf = (char *)malloc(buflen+bufalign)) == (char *)NULL)
    err("malloc");
  if (bufalign != 0)
    buf +=(bufalign - ((long int)buf % bufalign) + bufoffset) % bufalign;

  if (!onetflag && !maceflag) {
    if (trans) {
      fprintf(stdout,
	      "ttcp-t: buflen=%d, nbuf=%d, align=%d/%d, port=%d",
	      buflen, nbuf, bufalign, bufoffset, port);
      if (sockbufsize)
	fprintf(stdout, ", sockbufsize=%d", sockbufsize);
      fprintf(stdout, "  %s  -> %s\n", udp?"udp":"tcp", host);
    } else {
      fprintf(stdout,
	      "ttcp-r: buflen=%d, nbuf=%d, align=%d/%d, port=%d",
	      buflen, nbuf, bufalign, bufoffset, port);
      if (sockbufsize)
	fprintf(stdout, ", sockbufsize=%d", sockbufsize);
      fprintf(stdout, "  %s\n", udp?"udp":"tcp");
    }

    if ((fd = socket(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
      err("socket");
    mes("socket");

    if(!trans)  {
      if (bind(fd, (struct sockaddr *) &sinme, sizeof(sinme)) < 0)
        err("bind");
    }

#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
    if (sockbufsize) {
      if (trans) {
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
		       sizeof sockbufsize) < 0)
	  err("setsockopt: sndbuf");
	mes("sndbuf");
      } else {
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
		       sizeof sockbufsize) < 0)
	  err("setsockopt: rcvbuf");
	mes("rcvbuf");
      }
    }
#endif
    if (!udp)  {
      #ifdef SIGPIPE
      signal(SIGPIPE, sigpipe);
      #endif
      if (trans) {
	/* We are the client if transmitting */
	if (options)  {
// #if defined(BSD42)
// 			if( setsockopt(fd, SOL_SOCKET, options, 0, 0) < 0)
// #else /* BSD43 */
	  if( setsockopt(fd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
// #endif
	    err("setsockopt");
	}
#ifdef TCP_NODELAY
	if (nodelay) {
	  struct protoent *p;
	  p = getprotobyname("tcp");
	  if( p && setsockopt(fd, p->p_proto, TCP_NODELAY, 
			      &one, sizeof(one)) < 0)
	    err("setsockopt: nodelay");
	  mes("nodelay");
	}
#endif
	if(connect(fd, (struct sockaddr *)&sinhim, sizeof(sinhim)) < 0)
	  err("connect");
	mes("connect");
      } else {
	/* otherwise, we are the server and 
	 * should listen for the connections
	 */
	listen(fd,0);   /* allow a queue of 0 */
	if(options)  {
	  if( setsockopt(fd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
	    err("setsockopt");
	}
	fromlen = sizeof(frominet);
	domain = AF_INET;
	if((fd=accept(fd, (struct sockaddr *)&frominet, &fromlen)) < 0)
	  err("accept");
	{
	  struct sockaddr_in peer;
	  socklen_t peerlen = sizeof(peer);
	  if (getpeername(fd, (struct sockaddr *) &peer, 
			  &peerlen) < 0) {
	    err("getpeername");
	  }
	  fprintf(stderr,"ttcp-r: accept from %s\n", 
		  inet_ntoa(peer.sin_addr));
	}
      }
    }
  }
//   else if (onetflag) {
// //     cout << "binding to " << local << endl;
//     configStart = TimeUtil::timeu();
//     onet_configure();
//     if (!onetgateway.empty()) {
//       params::set("onet_server", onetgateway);
//     }
//     configEnd = TimeUtil::timeu();
//     initStart = TimeUtil::timeu();
//     if (onet_init(local, on_addr::null) < 0) {
//       onet_perror("onet_init");
//     }
//     initEnd = TimeUtil::timeu();
// //     cout << "opening " << local << " for reading" << endl;

//     localOpenStart = TimeUtil::timeu();
//     if (open(local, onet::O_READ) < 0) {
//       onet_perror("open");
//     }
//     localOpenEnd = TimeUtil::timeu();
//     if (trans) {
// //       cout << "opening " << dest << " for writing" << endl;
//       openStart = TimeUtil::timeu();
//       if (open(dest, onet::O_WRITE) < 0) {
// 	onet_perror("open");
//       }
//       openEnd = TimeUtil::timeu();
//     }
//   }

  connectEnd = TimeUtil::timeu();

//   if (!(onetflag || maceflag) || trans) {
  if (!maceflag || trans) {
    prep_timer();
  }
  errno = 0;
  if (sinkmode) {      
    register int cnt;
    if (trans)  {
      pattern( buf, buflen );
//       if (onetflag || maceflag) {
      if (maceflag) {
	onetbuf = std::string(buf, buflen);
      }
      if(udp)  (void)Nwrite( fd, buf, 4 ); /* rcvr start */
      while (nbuf-- && Nwrite(fd,buf,buflen) == buflen)
	nbytes += buflen;
      if(udp)  (void)Nwrite( fd, buf, 4 ); /* rcvr end */
    } else {
      if (udp) {
	while ((cnt=Nread(fd,buf,buflen)) > 0)  {
	  static int going = 0;
	  if( cnt <= 4 )  {
	    if( going )
	      break;	/* "EOF" */
	    going = 1;
	    prep_timer();
	  } else {
	    nbytes += cnt;
	  }
	}
      } else {
	if (maceflag) {
	  while (!macedone) {
	    SysUtil::sleepm(3);
	  }
	}
	else {
	  while ((cnt=Nread(fd,buf,buflen)) > 0)  {
	    nbytes += cnt;
	  }
	}
      }
    }
  } else {
    register int cnt;
    if (trans)  {
      while((cnt=read(0,buf,buflen)) > 0 &&
	    Nwrite(fd,buf,cnt) == cnt)
	nbytes += cnt;
    }  else  {
      while((cnt=Nread(fd,buf,buflen)) > 0 &&
	    write(1,buf,cnt) == cnt)
	nbytes += cnt;
    }
  }

//   if (trans && onetflag) {
//     close(dest);
//   }
  if ((trans && maceflag) || !maceforward.isNullAddress()) {
    uint64_t now = TimeUtil::timeu();
    while (transport->hasOutgoingBufferedData() || sleepflag) {
      SysUtil::sleepm(5);
    }
    uint64_t next = TimeUtil::timeu();
    cout << "spent " << next - now << " waiting for buffered data" << endl;
  }

//   if (onetflag) {
//     onet_exit();
//   }

//   if (!onetflag && !maceflag) {
  if (!maceflag) {
    if(errno) err("IO");
  }

  (void)read_timer(stats,sizeof(stats));
  if(udp&&trans)  {
    (void)Nwrite( fd, buf, 4 ); /* rcvr end */
    (void)Nwrite( fd, buf, 4 ); /* rcvr end */
    (void)Nwrite( fd, buf, 4 ); /* rcvr end */
    (void)Nwrite( fd, buf, 4 ); /* rcvr end */
  }
  if( cput <= 0.0 )  cput = 0.001;
  if( realt <= 0.0 )  realt = 0.001;
  fprintf(stdout,
	  "ttcp%s: %" PRIu64 " bytes in %.2f real seconds = %s/sec +++\n",
	  trans?"-t":"-r",
	  nbytes, realt, outfmt(((double)nbytes)/realt));
  if (verbose) {
    fprintf(stdout,
	    "ttcp%s: %" PRIu64 " bytes in %.2f CPU seconds = %s/cpu sec\n",
	    trans?"-t":"-r",
	    nbytes, cput, outfmt(((double)nbytes)/cput));
  }
  fprintf(stdout,
	  "ttcp%s: %lu I/O calls, msec/call = %.2f, calls/sec = %.2f\n",
	  trans?"-t":"-r",
	  numCalls,
	  1024.0 * realt/((double)numCalls),
	  ((double)numCalls)/realt);
  fprintf(stdout,"ttcp%s: %s\n", trans?"-t":"-r", stats);
  if (verbose) {
    fprintf(stdout,
	    "ttcp%s: buffer address %p\n",
	    trans?"-t":"-r",
	    buf);
  }

  cout << "config time = " << configEnd - configStart << " us" << endl;
  cout << "init time = " << initEnd - initStart << " us" << endl;
  cout << "local open time = " << localOpenEnd - localOpenStart << " us" << endl;
  cout << "open time = " << openEnd - openStart << " us" << endl;

  uint64_t connectTime = connectEnd - connectStart;
  cout << "connect time = " << connectTime << " us" << endl;
//   cout << "cstart=" << connectStart << " cend=" << connectEnd << endl;
  cout << "send time = " << sendTime << " us" << endl;

  if (maceflag) {
    transport->maceExit();
    Scheduler::haltScheduler();
  }
  exit(0);
}

void printUsage() {
  fprintf(stderr,"%s", Usage);
  exit(1);
}  

void err(const char* s) 
{
  fprintf(stderr,"ttcp%s: ", trans?"-t":"-r");
  perror(s);
  fprintf(stderr,"errno=%d\n",errno);
  exit(1);
}

void mes(const char* s)
{
  fprintf(stderr,"ttcp%s: %s\n", trans?"-t":"-r", s);
}

void pattern(char* cp, int cnt)
{
  register char c;
  c = 0;
  while( cnt-- > 0 )  {
//     while( !isprint((c&0x7F)) )  c++;
    *cp++ = (c++&0x7F);
  }
}

char * outfmt(double b)
{
  static char obuf[50];
  switch (fmt) {
  case 'G':
    sprintf(obuf, "%.2f GB", b / 1024.0 / 1024.0 / 1024.0);
    break;
  default:
  case 'K':
    sprintf(obuf, "%.2f KB", b / 1024.0);
    break;
  case 'M':
    sprintf(obuf, "%.2f MB", b / 1024.0 / 1024.0);
    break;
  case 'g':
    sprintf(obuf, "%.2f Gbit", b * 8.0 / 1024.0 / 1024.0 / 1024.0);
    break;
  case 'k':
    sprintf(obuf, "%.2f Kbit", b * 8.0 / 1024.0);
    break;
  case 'm':
    sprintf(obuf, "%.2f Mbit", b * 8.0 / 1024.0 / 1024.0);
    break;
  }
  return obuf;
}

static struct	timeval time0;	/* Time at which timing started */
static struct	rusage ru0;	/* Resource utilization at the start */

static void prusage(struct rusage *r0, struct rusage *r1, struct timeval *e,
		    struct timeval *b, char *outp);
static void tvadd(struct timeval *tsum, struct timeval *t0, struct timeval *t1);
static void tvsub(struct timeval* tdiff, struct timeval* t1, struct timeval* t0);
static void psecs(long l, char* cp);

#if defined(SYSV)
/*ARGSUSED*/
static
void getrusage(int, ignored, struct rusage* ru)
{
  struct tms buf;

  times(&buf);

  /* Assumption: HZ <= 2147 (LONG_MAX/1000000) */
  ru->ru_stime.tv_sec  = buf.tms_stime / HZ;
  ru->ru_stime.tv_usec = ((buf.tms_stime % HZ) * 1000000) / HZ;
  ru->ru_utime.tv_sec  = buf.tms_utime / HZ;
  ru->ru_utime.tv_usec = ((buf.tms_utime % HZ) * 1000000) / HZ;
}

/*ARGSUSED*/
static 
void gettimeofday(struct timeval *tp, struct timezone *zp)
{
  tp->tv_sec = time(0);
  tp->tv_usec = 0;
}
#endif /* SYSV */

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
  gettimeofday(&time0, (struct timezone *)0);
  getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 * 
 */
double
read_timer(char *str, int len)
{
  struct timeval timedol;
  struct rusage ru1;
  struct timeval td;
  struct timeval tend, tstart;
  char line[132];

  getrusage(RUSAGE_SELF, &ru1);
  gettimeofday(&timedol, (struct timezone *)0);
  prusage(&ru0, &ru1, &timedol, &time0, line);
  strncpy( str, line, len );

  /* Get real time */
  tvsub( &td, &timedol, &time0 );
  realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

  /* Get CPU time (user+sys) */
  tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
  tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
  tvsub( &td, &tend, &tstart );
  cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
  if( cput < 0.00001 )  cput = 0.00001;
  return( cput );
}

static void
prusage(struct rusage *r0, struct rusage *r1, struct timeval *e, struct timeval *b,
	char *outp)
{
  struct timeval tdiff;
  register time_t t;
  register const char *cp;
  register int i;
  int ms;

  t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
  ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}
#if defined(SYSV)
  cp = "%Uuser %Ssys %Ereal %P";
#else
#if defined(sgi)		/* IRIX 3.3 will show 0 for %M,%F,%R,%C */
  cp = "%Uuser %Ssys %Ereal %P %Mmaxrss %F+%Rpf %Ccsw";
#else
  cp = "%Uuser %Ssys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Ccsw";
#endif
#endif
  for (; *cp; cp++)  {
    if (*cp != '%')
      *outp++ = *cp;
    else if (cp[1]) switch(*++cp) {

    case 'U':
      tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
      sprintf(outp,"%ld.%01ld", tdiff.tv_sec, (long int) tdiff.tv_usec/100000);
      END(outp);
      break;

    case 'S':
      tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
      sprintf(outp,"%ld.%01ld", tdiff.tv_sec, (long int) tdiff.tv_usec/100000);
      END(outp);
      break;

    case 'E':
      psecs(ms / 100, outp);
      END(outp);
      break;

    case 'P':
      sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
      END(outp);
      break;

#if !defined(SYSV)
    case 'W':
      i = r1->ru_nswap - r0->ru_nswap;
      sprintf(outp,"%d", i);
      END(outp);
      break;

    case 'X':
      sprintf(outp,"%ld", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
      END(outp);
      break;

    case 'D':
      sprintf(outp,"%ld", t == 0 ? 0 :
	      (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
      END(outp);
      break;

    case 'K':
      sprintf(outp,"%ld", t == 0 ? 0 :
	      ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
	       (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
      END(outp);
      break;

    case 'M':
      sprintf(outp,"%ld", r1->ru_maxrss/2);
      END(outp);
      break;

    case 'F':
      sprintf(outp,"%ld", r1->ru_majflt-r0->ru_majflt);
      END(outp);
      break;

    case 'R':
      sprintf(outp,"%ld", r1->ru_minflt-r0->ru_minflt);
      END(outp);
      break;

    case 'I':
      sprintf(outp,"%ld", r1->ru_inblock-r0->ru_inblock);
      END(outp);
      break;

    case 'O':
      sprintf(outp,"%ld", r1->ru_oublock-r0->ru_oublock);
      END(outp);
      break;
    case 'C':
      sprintf(outp,"%ld+%ld", r1->ru_nvcsw-r0->ru_nvcsw,
	      r1->ru_nivcsw-r0->ru_nivcsw );
      END(outp);
      break;
#endif /* !SYSV */
    }
  }
  *outp = '\0';
}

static void
tvadd(struct timeval *tsum, struct timeval *t0, struct timeval *t1)
{

  tsum->tv_sec = t0->tv_sec + t1->tv_sec;
  tsum->tv_usec = t0->tv_usec + t1->tv_usec;
  if (tsum->tv_usec > 1000000)
    tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(struct timeval* tdiff, struct timeval* t1, struct timeval* t0)
{

  tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
  tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
  if (tdiff->tv_usec < 0)
    tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
psecs(long l, char* cp)
{
  register int i;

  i = l / 3600;
  if (i) {
    sprintf(cp,"%d:", i);
    END(cp);
    i = l % 3600;
    sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
    END(cp);
  } else {
    i = l;
    sprintf(cp,"%d", i / 60);
    END(cp);
  }
  i %= 60;
  *cp++ = ':';
  sprintf(cp,"%d%d", i / 10, i % 10);
}

/*
 *			N R E A D
 */
int Nread(int fd, char* buf, int count)
{
//   if (onetflag) {
//     on_addr src;
//     int r = read(local, src, onetbuf);
//     if (numCalls == 0) {
//       prep_timer();
//     }
//     numCalls++;

// //     cout << "read " << r << " " << numCalls << endl;
//     if (r <= 0) {
//       return r;
//     }
//     return onetbuf.size();
//   }

  struct sockaddr_in from;
  socklen_t len = sizeof(from);
  register int cnt;
  if( udp )  {
    cnt = recvfrom( fd, buf, count, 0,(struct sockaddr *)&from,
		    &len );
    numCalls++;
  } else {
    if( b_flag )
      cnt = mread( fd, buf, count );	/* fill buf */
    else {
      cnt = read( fd, buf, count );
      numCalls++;
    }
    if (touchdata && cnt > 0) {
      register int c = cnt, sum;
      register char *b = buf;
      while (c--)
	sum += *b++;
    }
  }
  return(cnt);
}

/*
 *			N W R I T E
 */
int Nwrite(int fd, void* buf, int count)
{
  uint64_t now = TimeUtil::timeu();
//   if (onetflag) {
//     numCalls++;
//     int r = write(dest, onetbuf);
// //     cout << "write " << r << " " << numCalls << endl;
//     if (r <= 0) {
//       return r;
//     }
//     return onetbuf.size();
//   }
//   else
  if (maceflag) {
    numCalls++;
    int r = transport->send(macedest, onetbuf);
//     cout << "write " << r << " " << numCalls << " to " << macedest << endl;
    sendTime += TimeUtil::timeu() - now;
    return r;
  }

  register int cnt;
  if( udp )  {
  again:
    cnt = sendto( fd, buf, count, 0, (struct sockaddr *) &sinhim,
		  sizeof(sinhim) );
    numCalls++;
    if( cnt<0 && errno == ENOBUFS )  {
      delay(18000);
      errno = 0;
      goto again;
    }
  } else {
    cnt = write( fd, buf, count );
    numCalls++;
  }
  sendTime += TimeUtil::timeu() - now;
  return(cnt);
}

void
delay(int us)
{
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = us;
  (void)select( 1, 0, 0, 0, &tv );
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
mread(int fd, char* bufp, unsigned n)
{
  register unsigned	count = 0;
  register int		nread;

  do {
    nread = read(fd, bufp, n-count);
    numCalls++;
    if(nread < 0)  {
      perror("ttcp_mread");
      return(-1);
    }
    if(nread == 0)
      return((int)count);
    count += (unsigned)nread;
    bufp += nread;
  } while(count < n);

  return((int)count);
}
