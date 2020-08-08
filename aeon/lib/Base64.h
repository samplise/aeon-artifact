/* 
 * Base64.h : part of the Mace toolkit for building distributed systems
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
//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//
//* Reformated by James W. Anderson.  Added declarations to use
//* enhanced version.
//*********************************************************************

#ifndef BASE64_H
#define BASE64_H

/**
 * \file Base64.h
 * \brief code to compute Base64 of strings.  Not originally written for Mace.
 *
 * Copyright notice:
 * \verbatim
 * C_Base64 - a simple base64 encoder and decoder.
 * 
 *   Copyright (c) 1999, Bob Withers - bwit@pobox.com
 * 
 * This code may be freely used for any purpose, either personal
 * or commercial, provided the authors copyright notice remains
 * intact.
 * 
 * Reformated by James W. Anderson.  Added declarations to use
 * enhanced version.
 * \endverbatim
 */

#include <string>

/// class to encode and decode strings using Base64
class Base64 {
public:
  static std::string encode(const std::string& data); ///< returns the Base64 version of \c data as a string.
  static std::string decode(const std::string& data); ///< returns the decoded version of Base64 encoded \c data as a string.

  static bool isPrintable(const std::string& s); ///< Tests a string \c s to see if all the characters are printable (whether to call encode())).

  static const std::string Base64Table;

private:
  static const std::string::size_type DecodeTable[];
  static const char fillchar;
  static const std::string::size_type np;
};

#endif // BASE64_H
