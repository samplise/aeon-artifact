/* 
 * massert.h : part of the Mace toolkit for building distributed systems
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

/**
 * \file massert.h
 * \brief Defines Mace assertion statements to use instead of assert or cassert.
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef ASSERT_ONCE
#define ASSERT_ONCE

/// ASSERT_EXIT_MECHANISM tells how an ASSERT or ABORT should terminate execution.  Two available options are abort and segfault.
// #define ASSERT_EXIT_MECHANISM char* __a = NULL; *__a = 5; //segfault
#define ASSERT_EXIT_MECHANISM ::abort(); //abort

/**
 * \def ASSERT(x) 
 * \brief Tests property \c x and terminates execution if it is false.  
 *
 * Prints error message to stdout, stderr, and Log::err().  Uses
 * ASSERT_EXIT_MECHANISM to exit.
 * \param x property to be tested.
 */

/**
 * \def ASSERTMSG(x, y)
 * \brief Tests property \c x and terminates execution with message \c y if it is false.  
 *
 * Prints error message \c y to stdout, stderr, and Log::err().  Uses
 * ASSERT_EXIT_MECHANISM to exit.  
 * \param x property to be tested.
 * \param y message to be printed.
 */

/**
 * \def ABORT(x)
 * \brief Terminates execution, printing error message \c y to stdout, stderr, and
 * Log::err().  
 *
 * Uses ASSERT_EXIT_MECHANISM to exit.  
 * \param x message to be printed.
 */

#define OUTPUT_FORMAT(m,x) "%s(%s) in file %s (included from %s), line %d method %s\n",m,#x,__FILE__,__BASE_FILE__,__LINE__,__PRETTY_FUNCTION__
#define OUTPUT_STREAM(m,x) m "(" #x ") in file " __FILE__ " (included from " __BASE_FILE__ "), line " << __LINE__ << " method " << __PRETTY_FUNCTION__ 

#endif //ASSERT_ONCE

#ifdef __LOG_H_DONE
#ifdef ABORT
#undef ABORT
#endif //ABORT
#define ABORT(x) \
   { \
     printf(OUTPUT_FORMAT("Abort",x));\
     fprintf(stderr, OUTPUT_FORMAT("Abort",x));\
     Log::err() << OUTPUT_STREAM("Abort",x) << Log::endl;\
     ASSERT_EXIT_MECHANISM \
   }
     
#ifdef ASSERT
#undef ASSERT
#endif //ASSERT
#define ASSERT(x) \
   { \
     if(x) {}\
     else {\
       printf(OUTPUT_FORMAT("Assert Failed",x));\
       fprintf(stderr, OUTPUT_FORMAT("Assert Failed",x));\
       Log::err() << OUTPUT_STREAM("Assert Failed",x) << Log::endl;\
       ASSERT_EXIT_MECHANISM \
     }\
   }
     
#ifdef ASSERTMSG
#undef ASSERTMSG
#endif //ASSERTMSG
#define ASSERTMSG(x,y) \
   { \
     if(x) {}\
     else {\
       printf(OUTPUT_FORMAT("Assert Failed",#y ": " #x));\
       fprintf(stderr, OUTPUT_FORMAT("Assert Failed",#y ": " #x));\
       Log::err() << OUTPUT_STREAM("Assert Failed",#y ": " #x) << Log::endl;\
       ASSERT_EXIT_MECHANISM \
     }\
   }
#else //__LOG_H_DONE
#ifndef ABORT
#define ABORT(x) \
   { \
     printf(OUTPUT_FORMAT("Abort",x));\
     fprintf(stderr, OUTPUT_FORMAT("Abort",x));\
     ASSERT_EXIT_MECHANISM \
   }
#endif //ABORT
     
#ifndef ASSERT
#define ASSERT(x) \
   { \
     if(x) {}\
     else {\
       printf(OUTPUT_FORMAT("Assert Failed", x));\
       fprintf(stderr, OUTPUT_FORMAT("Assert Failed", x));\
       ASSERT_EXIT_MECHANISM \
     }\
   }
#endif //ASSERT
     
#ifndef ASSERTMSG
#define ASSERTMSG(x,y) \
   { \
     if(x) {}\
     else {\
       printf(OUTPUT_FORMAT("Assert Failed", #y ": " #x));\
       fprintf(stderr, OUTPUT_FORMAT("Assert Failed", #y ": " #x));\
       ASSERT_EXIT_MECHANISM \
     }\
   }
#endif //ASSERTMSG
#endif //__LOG_H_DONE
