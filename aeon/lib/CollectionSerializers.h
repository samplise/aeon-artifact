/* 
 * CollectionSerializers.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson, Ryan Braud
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
#ifndef __COLLECTION_SERIALIZERS_H
#define __COLLECTION_SERIALIZERS_H

#include "Serializable.h"
#include "Log.h"
#include "XmlRpcCollection.h"

/**
 * \file CollectionSerializers.h
 * \brief a set of template classes which know how to serialize various collection types generically
 */

namespace mace {

/// Generic class for serializing arrays
/**
 * Arrays have known and fixed sizes with preallocated elements, which can and
 * should be taken advantage of during serialization.
 */
template<typename Value> 
class _SerializeArray : public Serializable {
  public:
    /// SerializeArray must be able to iterate over elements for deserialization
    class iterator {
      public:
        virtual bool isValid() = 0; ///< returns true if the iterator points to a valid element
        virtual void next() = 0; ///< moves the iterator to the next element
        virtual Value& getValue() = 0; ///< gets a mutable reference to the iterator element
        virtual ~iterator() {}
    };
    /// SerializeArray must be able to iterate over the elements in a const fashion for serialization
    class const_iterator {
      public:
        virtual bool isValid() = 0; ///< returns true of the iterator points to a valid element
        virtual void next() = 0; ///< moves the iterator to the next element
        virtual const Value& getValue() = 0; ///< gets a const reference to the iterator element
        virtual ~const_iterator() {}
    };

  protected:
  virtual iterator* getIterator() { ABORT("getIterator"); } ///< virtual method implemented by actual collection to return a SerializeArray iterator.
  virtual const_iterator* getIterator() const { ABORT("getIterator"); } ///< virtual method implemented by actual collection to return a SerializeArray const_iterator.

  public: 
    virtual void sqlize(LogNode* node) const {
      int index = node->nextIndex();
      std::string valueTable = node->logName + "_values";
      
      const_iterator* i = getIterator();
      int j = 0;
      
      if (index == 0) {
	std::ostringstream out;
	
	out << "CREATE TABLE " << node->logName << " (_id INT, index INT, "
	    << "values INT, PRIMARY KEY(_id, index));" << std::endl;
	Log::logSqlCreate(out.str(), node);
	node->children = new LogNode*[1];
	LogNode* child0 = new LogNode(valueTable, Log::sqlEventsLog);
// 	child0->out.open((valueTable + ".log").c_str());
	node->children[0] = child0;
      }
      for(; i->isValid(); i->next()) {
	int valueIndex = node->children[0]->nextId;
// 	std::ostringstream out;
	
// 	out << node->logId << "\t" << index << "\t" << j++ << "\t" 
// 	    << valueIndex << std::endl;
	fprintf(node->file, "%d\t%d\t%d\t%d\n", node->logId, index, j++, valueIndex);
// 	fwrite(out.str().c_str(), out.str().size(), 1, node->file);
	mace::sqlize(&(i->getValue()), node->children[0]);
      }
      delete i;
    }

    /// implement serialization by iterating over all elements and serializing them. 
    /**
     * Since an array has fixed size, no extra work is needed, since the
     * deserialization should read the same number of elements.
     */
    void serialize(std::string& str) const {
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        mace::serialize(str, &(i->getValue()));
      }
      delete i;
    }
    /// implement deserialization by iterator over (the fixed number of) elements and deserializing them.
    int deserialize(std::istream& in) throw(SerializationException) {
      int bytes = 0;
      iterator* i = getIterator();
      for (; i->isValid(); i->next()) {
        bytes += mace::deserialize(in, &(i->getValue()));
      }
      delete i;
      return bytes;
    }
    /// Support array deserialization of XML_RPC using \verbatim <array><data></data>...</array> \endverbatim
    int deserializeXML_RPC(std::istream& in) throw(SerializationException) {    
      long offset = in.tellg();

      //       obj.clear(); -- arrays would already be the right size.
      SerializationUtil::expectTag(in, "<array>");
      SerializationUtil::expectTag(in, "<data>");
      std::string tag = SerializationUtil::getTag(in);
      iterator* i = getIterator();
      for (; i->isValid(); i->next()) {
        SerializationUtil::expectTag(in, "<value>");
        mace::deserializeXML_RPC(in, &(i->getValue()), i->getValue());
        SerializationUtil::expectTag(in, "</value>");
        tag = SerializationUtil::getTag(in);
      }
      delete i;
      SerializationUtil::expectTag(in, "</data>");
      SerializationUtil::expectTag(in, "</array>");
      //   in >> skipws;
      return (long)in.tellg() - offset;
    }
    /// Support array serialization using \verbatim <array><data></data>...</array> \endverbatim
    void serializeXML_RPC(std::string& str) const throw(SerializationException) {
      str.append("<array><data>");
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        str.append("<value>");
        mace::serializeXML_RPC(str, &(i->getValue()), i->getValue());
        str.append("</value>");
      }
      delete i;
      str.append("</data></array>");
    }
};

/// wrapper around _SerializeArray to provide Boost concept checking giving good error messages.
template<typename Value> 
class SerializeArray : public _SerializeArray<Value> {
  BOOST_CLASS_REQUIRE(Value, mace, SerializeConcept);
};


/// Generic class for serializing lists
/**
 * Lists are like arrays, but for a variable number of elements.  Serialization
 * proceeds by allocating the right number of elements, then using array
 * serialization.  It avoids clearing all elements, required by some other data
 * structures (like sets).
 */
template<typename Value> 
class _SerializeList : public _SerializeArray<Value> {
  public:
  virtual void resize(size_t sz) { ABORT("resize"); }; ///< deserialization must be able to fix the number of elements
  virtual size_t size() const { ABORT("size"); }; ///< serialization needs to know the number of elements
  virtual void add(const Value& v) { ABORT("add"); } ///< XML_RPC cannot use efficient techniques, so will add elements as it reads them.

    /// serialize the size of the list, then use array serialization
    void serialize(std::string& str) const {
      uint32_t sz = (uint32_t)size();
      mace::serialize(str, &sz);
      _SerializeArray<Value>::serialize(str);
    }
    /// deserialize and set the size of the list, then use array deserialization
    int deserialize(std::istream& in) throw(SerializationException) {
      uint32_t sz;
      int bytes = sizeof(sz);

      mace::deserialize(in, &sz);
      resize(sz);

      bytes += _SerializeArray<Value>::deserialize(in);
      return bytes;
    }
    /// Support array XML_RPC serialization by clearing all elements, then adding elements as they are read
    int deserializeXML_RPC(std::istream& in) throw(SerializationException) {    
      long offset = in.tellg();

      resize(0);
      SerializationUtil::expectTag(in, "<array>");
      SerializationUtil::expectTag(in, "<data>");
      std::string tag = SerializationUtil::getTag(in);
      while (tag == "<value>") {
        Value v;
        mace::deserializeXML_RPC(in, &(v), v);
        add(v);
        SerializationUtil::expectTag(in, "</value>");
        tag = SerializationUtil::getTag(in);
      }

      if (tag != "</data>") {
        throw SerializationException("error parsing tag: expected </data>, parsed " + tag);
      }

      SerializationUtil::expectTag(in, "</array>");
      //   in >> skipws;
      return (long)in.tellg() - offset;
    }
};

/// wrapper around _SerializeList to provide Boost concept checking giving good error messages.
template<typename Value> 
class SerializeList : public _SerializeList<Value> {
  BOOST_CLASS_REQUIRE(Value, mace, SerializeConcept);
};

/// Generic class for serializing sets
/**
 * Sets are different from lists because there are no mutable iterators for
 * sets.  Deserialization must proceed by clearing elements then inserting new
 * ones.
 *
 * (a more efficient deserialization exists if the sets may be very similar to
 * begin with, by reading in elements and only inserting if new, only removing
 * if not in the serialized set.  This could be pursued in the future.)
 */
template<typename Value> 
class _SerializeSet : public Serializable {
  public:
    /// set serialization only requires const iteration, for serializing all the elements
    class const_iterator {
      public:
        virtual bool isValid() = 0; ///< whether the iterator points to a valid element.
        virtual void next() = 0; ///< advance the iterator
        virtual const Value& getValue() = 0; ///< return a const reference to the element
        virtual ~const_iterator() {}
    };

  protected:
  virtual const_iterator* getIterator() const { ABORT("getIterator"); } ///< implemented by the actual collection to return a _SerializeSet<Value>::const_iterator
  virtual void add(const Value& v) { ABORT("add"); } ///< implemented by the actual collection to add values to the set

  public: 
  virtual void clear() { ABORT("clear"); } ///< implementation should remove all elements from the set
  virtual size_t size() const { ABORT("size"); } ///< implementation should return the size of the set (for serialization purposes)

  virtual void sqlize(LogNode* node) const {
    int index = node->nextIndex();
    std::string valueTable = node->logName + "_values";
    
    const_iterator* i = getIterator();
    
    if (index == 0) {
      std::ostringstream out;
      
      out << "CREATE TABLE " << node->logName << " (_id INT, values INT, "
	  << "PRIMARY KEY(_id, values));" << std::endl;
      Log::logSqlCreate(out.str(), node);
      node->children = new LogNode*[1];
      LogNode* child0 = new LogNode(valueTable, Log::sqlEventsLog);
      node->children[0] = child0;
    }
    for(; i->isValid(); i->next()) {
      int valueIndex = node->children[0]->nextId;
//       std::ostringstream out;
      
//       out << node->logId << "\t" << index << "\t" << valueIndex << std::endl;
//       fwrite(out.str().c_str(), out.str().size(), 1, node->file);
      fprintf(node->file, "%d\t%d\t%d\n", node->logId, index, valueIndex);
      mace::sqlize(&(i->getValue()), node->children[0]);
    }
    delete i;
  }

    /// Serialize proceeds by serializing the size, and iterative over all elements serializing each one.
    void serialize(std::string& str) const {
      uint32_t sz = (uint32_t)size();
      mace::serialize(str, &sz);
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        mace::serialize(str, &(i->getValue()));
      }
      delete i;
    }
    /// Deserialization proceeds by clearing the elements, deserializing the size, and reading that many elements.
    int deserialize(std::istream& in) throw(SerializationException) {
      Value v;
      uint32_t sz;
      int bytes = sizeof(sz);
      //     this->resize(sz);

      clear();
      mace::deserialize(in, &sz);
      for (uint32_t i = 0; i < sz; i++) {
        bytes += mace::deserialize(in, &v);
        add(v);
        //multimap:   insert(std::pair<typename BaseMap::key_type, typename BaseMap::mapped_type>(T, d));
      }
      return bytes;
    }
    /// XML_RPC (De)Serialization proceeds by array methods
    int deserializeXML_RPC(std::istream& in) throw(SerializationException) {    
      long offset = in.tellg();

      clear();
      SerializationUtil::expectTag(in, "<array>");
      SerializationUtil::expectTag(in, "<data>");
      std::string tag = SerializationUtil::getTag(in);
      while (tag == "<value>") {
        Value v;
        mace::deserializeXML_RPC(in, &(v), v);
        add(v);
        SerializationUtil::expectTag(in, "</value>");
        tag = SerializationUtil::getTag(in);
      }

      if (tag != "</data>") {
        throw SerializationException("error parsing tag: expected </data>, parsed " + tag);
      }

      SerializationUtil::expectTag(in, "</array>");
      //   in >> skipws;
      return (long)in.tellg() - offset;
    }
    /// XML_RPC (De)Serialization proceeds by array methods
    void serializeXML_RPC(std::string& str) const throw(SerializationException) {
      str.append("<array><data>");
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        str.append("<value>");
        mace::serializeXML_RPC(str, &(i->getValue()), i->getValue());
        str.append("</value>");
      }
      delete i;
      str.append("</data></array>");
    }
};

/// wrapper around _SerializeSet to provide Boost concept checking giving good error messages.
template<typename Value> 
class SerializeSet : public _SerializeSet<Value> {
  BOOST_CLASS_REQUIRE(Value, mace, SerializeConcept);
};

/// Generic class for serializing maps
/**
 * Maps work like sets, but each element is a key and value.  Multimaps are
 * maps too, but won't work with XML_RPC serialization if there are actual
 * duplicate keys.  The runtime implementation behavior is unknown.
 */
template<typename Key, typename Value> 
class _SerializeMap : public Serializable {

  public:
    //class iterator {
    //  public:
    //    virtual bool isValid() = 0;
    //    virtual void next() = 0;
    //    virtual const typename Key& getKey() = 0;
    //    virtual typename Value& getValue() = 0;
    //};

    /// Const iteration is used for serializing map elements
    class const_iterator {
      public:
        virtual bool isValid() = 0; ///< returns whether the iterator points to a valid element
        virtual void next() = 0; ///< advances the iterator
        virtual const Key& getKey() = 0; ///< gets a const reference the key
        virtual const Value& getValue() = 0; ///< gets a const reference to the value
        virtual ~const_iterator() {}
    };

  protected:
  virtual Value& insertKey(const Key& k) { ABORT("insertKey"); } ///< inserts an element into the map with Key k, returning a reference to the Value for deserialization
  virtual const_iterator* getIterator() const { ABORT("getIterator"); } ///< gets an iterator

  public: 
  virtual void clear() { ABORT("clear"); } ///< called to clear all elements
  virtual size_t size() const { ABORT("size"); } ///< called to return the number of elements
  virtual void sqlize(LogNode* node) const {
    int index = node->nextIndex();
    std::string keyTable = node->logName + "_keys";
    std::string valueTable = node->logName + "_values";
    
    const_iterator* i = getIterator();
    
    if (index == 0) {
      std::ostringstream out;
      out << "CREATE TABLE " << node->logName << " (_id INT, keys INT, "
	  << "values INT, PRIMARY KEY(_id, keys));" << std::endl;
      Log::logSqlCreate(out.str(), node);
      
      node->children = new LogNode*[2];
      LogNode* child0 = new LogNode(keyTable, Log::sqlEventsLog);
//       child0->out.open((keyTable + ".log").c_str());
      node->children[0] = child0;
      LogNode* child1 = new LogNode(valueTable, Log::sqlEventsLog);
//       child1->out.open((valueTable + ".log").c_str());
      node->children[1] = child1;

    }
    for(; i->isValid(); i->next()) {
      int keyIndex = node->children[0]->nextId;
      int valueIndex = node->children[1]->nextId;
//       std::ostringstream out;
      
//       out << node->logId << "\t" << index << "\t" << keyIndex << "\t" 
// 	  << valueIndex << std::endl;
//       fwrite(out.str().c_str(), out.str().size(), 1, node->file);
      fprintf(node->file, "%d\t%d\t%d\t%d\n", node->logId, index, keyIndex, valueIndex);
      mace::sqlize(&(i->getKey()), node->children[0]);
      mace::sqlize(&(i->getValue()), node->children[1]);
    }
    delete i;
  }
  
  /// serialization proceeds by serializing the size, then each element, key then value.
    void serialize(std::string& str) const {
      uint32_t sz = (uint32_t)size();
      mace::serialize(str, &sz);
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        mace::serialize(str, &(i->getKey()));
        mace::serialize(str, &(i->getValue()));
      }
      delete i;
    }
    /// deserialization proceeds by reading the size, clearing the map, then reading in key then value for size count
    int deserialize(std::istream& in) throw(SerializationException) {
      Key k;
      uint32_t sz;
      int bytes = sizeof(sz);

      clear();
      mace::deserialize(in, &sz);
      for (uint32_t i = 0; i < sz; i++) {
        bytes += mace::deserialize(in, &k);
        bytes += mace::deserialize(in, &insertKey(k));
        //multimap:   insert(std::pair<typename BaseMap::key_type, typename BaseMap::mapped_type>(T, d));
      }
      return bytes;
    }
    /// map XML_RPC serialization uses XML_RPC struct serialzation (key is a member name, value is the value)
    int deserializeXML_RPC(std::istream& in) throw(SerializationException) {    
      std::istream::pos_type offset = in.tellg();

      clear();
      //   std::cout << "attempting to parse <struct>" << std::endl;
      SerializationUtil::expectTag(in, "<struct>");
      //cout << "got struct" << endl;
      std::string tag = SerializationUtil::getTag(in);
      while (tag == "<member>") {
        //cout << "got tag  " << tag << endl;
        //     std::cout << "parsed tag " << tag << std::endl;
        Key k;
        //cout << "got member token " << endl;
        //       std::cout << "attempting to parse <name> " << tag << std::endl;
        SerializationUtil::expectTag(in, "<name>");
        k = KeyTraits<Key>::extract(in);
        Value& d = insertKey(k);
        //cout <<"extracted key as " << k << endl;
        //mace::deserializeXML_RPC(in, &k, k);
        SerializationUtil::expectTag(in, "</name>");
        SerializationUtil::expectTag(in, "<value>");
        mace::deserializeXML_RPC(in, &d, d);
        SerializationUtil::expectTag(in, "</value>");
        SerializationUtil::expectTag(in, "</member>");

        tag = SerializationUtil::getTag(in);
        //     std::cout << "parsed tag " << tag << std::endl;
      }

      if (tag != "</struct>") {
        throw SerializationException("error parsing tag: expected </struct>, parsed " + tag);
      }

      //   in >> skipws;
      return in.tellg() - offset;
    }
    /// map XML_RPC serialization uses XML_RPC struct serialzation (key is a member name, value is the value)
    void serializeXML_RPC(std::string& str) const throw(SerializationException) {
      if(!KeyTraits<Key>::isString() && !KeyTraits<Key>::isInt()) {
        throw SerializationException("Bad key type for map serialization");
      }
      str.append("<struct>");
      const_iterator* i = getIterator();
      for(; i->isValid(); i->next()) {
        str.append("<member>");
        str.append("<name>");
        KeyTraits<Key>::append(str, i->getKey());
        str.append("</name>");
        str.append("<value>");
        mace::serializeXML_RPC(str, &(i->getValue()), i->getValue());
        str.append("</value>");
        str.append("</member>");
      }
      delete i;
      str.append("</struct>");
    }
};

  /// wrapper around _SerializeMap to provide Boost concept checking giving good error messages.
  template<typename Key, typename Value>
  class SerializeMap : public _SerializeMap<Key, Value> {
    BOOST_CLASS_REQUIRE(Key, mace, SerializeConcept);
    BOOST_CLASS_REQUIRE(Value, mace, SerializeConcept);
  };

}

#endif //__COLLECTION_SERIALIZERS_H
