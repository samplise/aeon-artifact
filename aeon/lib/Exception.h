/* 
 * Exception.h : part of the Mace toolkit for building distributed systems
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
#include <string>
#include <map>
#include "Printable.h"

#ifndef EXCEPTION_H
#define EXCEPTION_H

/**
 * \file Exception.h
 * \brief Defines the set of exception classes used by most Mace libraries
 * \todo JWA, please double check these.
 * \todo We should remove unused Exceptions
 */

/// Base class for all Mace exceptions, which adds printability, and a common message field
class Exception : public std::exception, public mace::ToStringPrintable {
public:
  Exception() { }
  virtual ~Exception() throw() { }
  Exception(const std::string& m) : message(m) { }
  virtual const char* what() const throw() { return message.c_str(); } ///< return the error message
  virtual std::string toString() const { return message; } ///< format the exception as a string
  virtual void rethrow() const { throw *this; } ///< throw this exception (used to provide good static typing on re-thrown exceptions)

protected:
  std::string message;

}; // Exception

/// Exception during IO
class IOException : public Exception {
public:
  IOException(const std::string& m) : Exception(m) { }
  virtual ~IOException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // IOException

/// Exception in file handling (often FileUtil methods)
class FileException : public IOException {
public:
  FileException() : IOException("") { }
  FileException(const std::string& m) : IOException(m) { }
  virtual ~FileException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // FileException

/// Exception thrown when the file type is unknown or erroneous
class BadFileTypeException : public FileException {
public:
  BadFileTypeException(const std::string& m) : FileException(m) { }
  virtual ~BadFileTypeException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // BadFileTypeException

/// Exception to encapsulate E_NOTFOUND 
class FileNotFoundException : public FileException {
public:  
  FileNotFoundException(const std::string& m) : FileException(m) { }
  virtual ~FileNotFoundException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // FileNotFoundException

/// Exception indicating the specified path is invalid or not well formed
class InvalidPathException : public FileException {
public:
  InvalidPathException(const std::string& m) : FileException(m) { }
  virtual ~InvalidPathException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // InvalidPathException

/// Exception to encapsulate E_ACCESS
class PermissionAccessException : public FileException {
public:
  PermissionAccessException(const std::string& m) : FileException(m) { }
  virtual ~PermissionAccessException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // PermissionAccessException

/// Exception to indicate a link loop (recursion error)
class LinkLoopException : public FileException {
public:
  LinkLoopException(const std::string& m) : FileException(m) { }
  virtual ~LinkLoopException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // LinkLoopException

/// error on read
class ReadException : public IOException {
public:
  ReadException(const std::string& m) : IOException(m) { }
  virtual ~ReadException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // ReadException

/// error on write
class WriteException : public IOException {
public:
  WriteException(const std::string& m) : IOException(m) { }
  virtual ~WriteException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // WriteException

/// Exception for E_PIPE
class PipeClosedException : public WriteException {
public:
  PipeClosedException(const std::string& m) : WriteException(m) { }
  virtual ~PipeClosedException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // PipeClosedException

/// Exception when trying to compare two objects that cannot be compared.
class IncomparableException : public Exception {
public:
  IncomparableException(const std::string& m) : Exception(m) { }
  virtual ~IncomparableException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // IncomparableException

/// Thrown for an invalid Mace/network address
class AddressException : public Exception {
public:
  AddressException(const std::string& m) : Exception(m) { }
  virtual ~AddressException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // AddressException

/// Thrown when a given address is an unreachable private address
class UnreachablePrivateAddressException : public AddressException {
public:
  UnreachablePrivateAddressException(const std::string& m) : AddressException(m) { }
  virtual ~UnreachablePrivateAddressException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // UnreachablePrivateAddressException

/// Thrown when a transition is called on an exited service (state == exited)
class ExitedException : public Exception {
public:
  ExitedException(const std::string& m) : Exception(m) {}
  virtual ~ExitedException() throw() {}
  virtual void rethrow() const { throw *this; }
}; // ExitedException

#endif // EXCEPTION_H

