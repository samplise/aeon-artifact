/* 
 * Printable.h : part of the Mace toolkit for building distributed systems
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
#ifndef __PRINTABLE_H
#define __PRINTABLE_H

#include <string>
#include <sstream>
#include <iomanip>
#include <ctype.h>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <list>

#include "hash_string.h"
#include "Base64.h"

/**
 * \file Printable.h
 * \brief defines the mace methods for printing just about anything.  Used by generated code, and from Mace Collections.
 */

namespace mace {

/**
 * \brief declares the Printable interface, for objects which know how to print themselves.
 *
 * Methods exist for toString() printable, to return a convenient string.
 * However, for performance, it also supports print() which prints right onto
 * an ostream.
 *
 * toXml exists looking forward, but isn't implemented yet by nearly any types.
 */

  class PrintNode;
  typedef std::list<PrintNode> PrintNodeList;

  class PrintNode  {
  public:
    PrintNode() { }
    virtual ~PrintNode() { }

    PrintNode(const std::string& name, const std::string& type, const std::string& value = std::string()) :
      name(name), type(type), value(value) { }

    void addChild(const PrintNode& n) { children.push_back(n); }
    const PrintNodeList& getChildren() const { return children; }
    const std::string& getName() const { return name; }
    const std::string& getType() const { return type; }
    const std::string& getValue() const { return value; }

  protected:
    std::string name;
    std::string type;
    std::string value;
    PrintNodeList children;
  };


  class Printable {
  public:
    virtual std::string toString() const = 0; ///< return this object as a string
    /// return this object as a state string.  
    /**
     * A state string doesn't contain things like timestamps which vary from run to run without meaning anything
     */
    virtual std::string toStateString() const = 0; 
    virtual std::string toXml() const = 0; ///< return the object in Xml format (Not XML_RPC, but more free Xml)
    virtual void print(std::ostream& printer) const = 0; ///< print the object string to \c printer
    virtual void printState(std::ostream& printer) const = 0; ///< print the object's state string to \c printer
    virtual void printNode(PrintNode& printer, const std::string& name) const = 0;
    virtual void printXml(std::ostream& printer) const = 0; ///< print the object's Xml representation to \c printer
    virtual ~Printable() {}
  };

/**
 * \brief Printable helper class which handles the toString methods by just implementing the print methods.
 */
  class PrintPrintable : virtual public Printable {
  public:
    std::string toString() const {
      std::ostringstream out;
      print(out);
      return out.str();
    }
    std::string toStateString() const {
      std::ostringstream out;
      printState(out);
      return out.str();
    }
    std::string toXml() const {
      std::ostringstream out;
      printXml(out);
      return out.str();
    }
    virtual void printState(std::ostream& printer) const {
      print(printer);
    }
    virtual void printXml(std::ostream& printer) const {
      printer << "<generic>";
      print(printer);
      printer << "</generic>";
    }
    using Printable::print;
    virtual void printNode(PrintNode& printer, const std::string& name) const {
      printer.addChild(PrintNode(name, std::string(), toString()));
    }

    virtual ~PrintPrintable() {}
  };

/**
 * \brief Printable helper class which handles the print methods by just implementing the toString methods.
 * \warning less efficient than PrintPrintable for printing
 */
  class ToStringPrintable : virtual public Printable {
  public:
    virtual void printNode(PrintNode& printer, const std::string& name) const {
      printer.addChild(PrintNode(name, std::string(), toString()));
    }
    void print(std::ostream& printer) const {
      printer << toString();
    }
    void printState(std::ostream& printer) const {
      printer << toStateString();
    }
    void printXml(std::ostream& printer) const {
      printer << toXml();
    }
    virtual std::string toStateString() const {
      return toString();
    }
    virtual std::string toXml() const {
      return toString();
    }
    virtual ~ToStringPrintable() {}
  };

/// simple method to do nothing (for use by macro to compile out printing)
  template<typename S>
  const int noConvertToString(const S* dud) { return 0; }

/**
 * \brief The printItem methods print the item passed in to the ostream.  
 *
 * Various sub-methods exist for all basic types, but also one for a generic
 * Printable object.  All others will be caught by this form, which prints a
 * non-printable warning.
 *
 * Primary use of this method is to support generated code and collections,
 * which try to print things which may in fact not support printing.  Calling
 * this method works in all cases, though it prints a warning in some.
 */
  inline void printItem(std::ostream& out, const void* pitem) {
    //   out << "[NON-PRINTABLE: address=" << std::hex << (int)pitem << " size=" << sizeof(S) << /*" typeid=" << typeid(S) <<  " hex=" << toHex(std::string((const char*)pitem, sizeof(S))) << */ "]";
    std::ios::fmtflags f(out.flags());
    out << std::hex;
    out << "[NON-PRINTABLE: ptr=" << pitem << "]";
    out.flags(f);
  }

  inline void printItem(PrintNode& pr, const std::string& name, const void* pitem) {
    std::ostringstream os;
    os << std::hex;
    os << pitem;
    pr.addChild(PrintNode(name, "void*", os.str()));
  }

  inline void printItem(std::ostream& out, const bool* pitem) {
    out << (*pitem?"true":"false");
  }

  inline void printItem(PrintNode& pr, const std::string& name, const bool* pitem) {
    pr.addChild(PrintNode(name, "bool", *pitem ? "true" : "false"));
  }

  inline void printItem(std::ostream& out, const double* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const double* pitem) {
    pr.addChild(PrintNode(name, "double", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const float* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const float* pitem) {
    pr.addChild(PrintNode(name, "float", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const int* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const int* pitem) {
    pr.addChild(PrintNode(name, "int", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const char* pitem) {
    out << (int)*pitem;
    if(isprint(*pitem)) {
      out << " '" << (*pitem) << "'";
    }
  }

  inline void printItem(PrintNode& pr, const std::string& name, const char* pitem) {
    std::ostringstream os;
    os << (int)*pitem;
    if (isprint(*pitem)) {
      os << " '" << (*pitem) << "'";
    }
    pr.addChild(PrintNode(name, "char", os.str()));
  }

  inline void printItem(std::ostream& out, const unsigned char* pitem) {
    out << (int)*pitem;
    if(isprint(*pitem)) {
      out << " '" << (*pitem) << "'";
    }
  }

  inline void printItem(PrintNode& pr, const std::string& name, const unsigned char* pitem) {
    std::ostringstream os;
    os << (int)*pitem;
    if (isprint(*pitem)) {
      os << " '" << (*pitem) << "'";
    }
    pr.addChild(PrintNode(name, "char", os.str()));
  }

  inline void printItem(std::ostream& out, const unsigned int* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const unsigned int* pitem) {
    pr.addChild(PrintNode(name, "uint", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const long int* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const long int* pitem) {
    pr.addChild(PrintNode(name, "long", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const short* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const short* pitem) {
    pr.addChild(PrintNode(name, "short", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const unsigned short* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const unsigned short* pitem) {
    pr.addChild(PrintNode(name, "ushort", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const unsigned long long* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const unsigned long long* pitem) {
    pr.addChild(PrintNode(name, "ullong", boost::lexical_cast<std::string>(*pitem)));
  }

  inline void printItem(std::ostream& out, const long long* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const long long* pitem) {
    pr.addChild(PrintNode(name, "llong", boost::lexical_cast<std::string>(*pitem)));
  }

#ifdef UINT64T_IS_NOT_ULONGLONG
  inline void printItem(std::ostream& out, const uint64_t* pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const uint64_t* pitem) {
    pr.addChild(PrintNode(name, "uint64", boost::lexical_cast<std::string>(*pitem)));
  }

// This does not appear to be necessary for x86_64 
// inline void printItem(std::ostream& out, const int64_t* pitem) {
//   out << *pitem;
// }
#endif

  void printItem(std::ostream& out, const std::string* pitem); 

  inline void printItem(PrintNode& pr, const std::string& name, const std::string* pitem) {
    static const hash_string hasher = hash_string();
    pr.addChild(PrintNode(name, "string",
			  Base64::isPrintable(*pitem) ? *pitem : "(str)[" + boost::lexical_cast<std::string>(hasher(*pitem)) + "]"));
  }

  inline void printItem(std::ostream& out, const char** pitem) {
    out << *pitem;
  }

  inline void printItem(PrintNode& pr, const std::string& name, const char** pitem) {
    pr.addChild(PrintNode(name, "string", *pitem));
  }

  inline void printItem(std::ostream& out, const Printable* pitem) {
    pitem->print(out);
  }

  inline void printItem(PrintNode& pr, const std::string& name, const Printable* pitem) {
    pitem->printNode(pr, name);
  }

  inline void printItem(std::ostream& out, const Printable** pitem) {
    // warning --- if pitem is an array, then only the first element will get printed
    (*pitem)->print(out);
  }

  inline void printItem(PrintNode& pr, const std::string& name, const Printable** pitem) {
    (*pitem)->printNode(pr, name);
  }

  template<typename S> 
  void printItem(std::ostream& out, const boost::shared_ptr<S>* pitem) {
    out << "shared_ptr(";
    mace::printItem(out, pitem->get());
    out << ")";
  }

  template<typename S>
  void printItem(PrintNode& pr, const std::string& name, const boost::shared_ptr<S>* pitem) {
    mace::printItem(pr, name, pitem->get());
  }

//   template<typename S, typename T>
//   void printItem(std::ostream& out, const typename std::map<S, T>::const_iterator* i) {
//     mace::printItem(out, (*i)->first, *((*i)->first));
//     out << "->";
//     mace::printItem(out, (*i)->second, *((*i)->second));
//   }

/// Generic method to print any object in its state format.
/**
 * the void* format of this method just uses printItem.  Since it's not
 * printable, printItem is as good as it gets.
 */
  template<typename S>
  void printState(std::ostream& out, const void* pitem, const S& item) {
    mace::printItem(out, &item);
  }

/// Generic method to print any object in its state format.
/**
 * calls the items printState method.
 */ 
  template<typename S>
  void printState(std::ostream& out, const Printable* pitem, const S& item) {
    item.printState(out); 
  }

  template<typename S> 
  void printState(std::ostream& out, const boost::shared_ptr<S>* pitem, const boost::shared_ptr<S>& item) {
    out << "shared_ptr(";
    mace::printState(out, item.get(), *item);
    out << ")";
  }

/// Generic method to convert a map to a string by iterating through its elements
  template<typename S>
  std::string mapToString(const S& m, bool newlines = false) {
    std::ostringstream os;
    for (typename S::const_iterator i = m.begin(); i != m.end(); i++) {
      os << "( " << convertToString(&(i->first), i->first) << " -> " << convertToString(&(i->second), i->second) << " )";
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    return os.str();
  }

  template<typename S>
  void printMap(PrintNode& pr, const std::string& name, const std::string& type, const S& b, const S& e) {
    PrintNode node(name, type);
    size_t n = 0;
    for (S i = b; i != e; i++) {
      std::string sn = boost::lexical_cast<std::string>(n);
      printItem(node, "key" + sn, &(i->first));
      printItem(node, "value" + sn, &(i->second));
      n++;
    }
    pr.addChild(node);
  }

/// Generic method to print a map (between two iterators) to a ostream object.
  template<typename S> 
  void printMap(std::ostream& os, const S& b, const S& e, bool newlines = false) {
    os << "[";
    if(newlines) { os << std::endl; } else { os << " "; }
    for (S i = b; i != e; i++) {
      os << "( ";
      printItem(os, &(i->first));
      os << " -> ";
      printItem(os, &(i->second));
      os << " )";
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    os << "]";
    if (newlines) {
      os << std::endl;
    }
  }

/// Generic method to print a map state (between two iterators) to a ostream object (calling printState for the key and value).
  template<typename S> 
  void printMapState(std::ostream& os, const S& b, const S& e, bool newlines = false) {
    os << "[";
    if(newlines) { os << std::endl; } else { os << " "; }
    for (S i = b; i != e; i++) {
      os << "( ";
      printState(os, &(i->first), i->first);
      os << " -> ";
      printState(os, &(i->second), i->second);
      os << " )";
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    os << "]";
    if (newlines) {
      os << std::endl;
    }
  }

/// Generic method to return a list as a string
  template<typename S> 
  std::string listToString(const S& l, bool newlines = false) {
    std::ostringstream os;
    for (typename S::const_iterator i = l.begin(); i != l.end(); i++) {
      os << convertToString(&(*i), *i);
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    return os.str();
  } // listToString

/// Generic method to return a list (between two iterators) as a string
  template<typename S> 
  std::string listToString(const S& b, const S& e, bool newlines = false) {
    std::ostringstream os;
    for (S i = b; i != e; i++) {
      os << convertToString(&(*i), *i);
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    return os.str();
  } // listToString

/// Generic method to print a list (between two iterators) to an ostream
  template<typename S> 
  void printList(std::ostream& os, const S& b, const S& e, bool newlines = false) {
    os << "[";
    if(newlines) { os << std::endl; } else { os << " "; }
    for (S i = b; i != e; i++) {
      printItem(os, &(*i));
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    os << "]";
    if (newlines) {
      os << std::endl;
    }
  } // listToString

  template<typename S>
  void printList(PrintNode& pr, const std::string& name, const std::string& type, const S& b, const S& e) {
    PrintNode node(name, type);
    size_t n = 0;
    for (S i = b; i != e; i++) {
      printItem(node, "item" + boost::lexical_cast<std::string>(n), &(*i));
      n++;
    }
    pr.addChild(node);
  }

/// Generic method to return a list of pointers as a string
  template<typename S>
  std::string pointerListToString(const S& l, bool newlines = false) {
    std::ostringstream os;
    for (typename S::const_iterator i = l.begin(); i != l.end(); i++) {
      os << convertToString(&(**i), **i);
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    return os.str();
  } // pointerListToString

/// Generic method to convert any object to a string
/**
 * Takes a pointer and a reference for historical reasons.  These days not
 * technically necessary.
 */
  template<typename S> 
  std::string convertToString(const S* pitem, const S& item) {
    std::ostringstream out;
    mace::printItem(out, pitem);
    return out.str();
  }

/// print a list (between two iterators) in state form
  template<typename S> 
  void printListState(std::ostream& os, const S& b, const S& e, bool newlines = false) {
    os << "[";
    if(newlines) { os << std::endl; } else { os << " "; }
    for (S i = b; i != e; i++) {
      printState(os, &(*i), *i);
      if (newlines) {
	os << std::endl;
      }
      else {
	os << " ";
      }
    }
    os << "]";
    if (newlines) {
      os << std::endl;
    }
  } 

/// print any object in xml format
  template<typename S>
  void printXml(std::ostream& os, const void* pitem, const S& item) {
    mace::printItem(os, &item);
  }

  template<typename S>
  void printXml(std::ostream& os, const Printable* pitem, const S& item) {
    pitem->printXml(os);
  }

} // namespace mace

/// Support the output operator to ostream for all mace::Printable objects.
inline std::ostream& operator<<(std::ostream& o, const mace::Printable& p) {
  p.print(o);
  return o;
}

#endif //__PRINTABLE_H

