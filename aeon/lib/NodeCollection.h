/* 
 * NodeCollection.h : part of the Mace toolkit for building distributed systems
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
// #include <algorithm>
#include <float.h>
#include <limits.h>
#include "mace.h"
// #include "m_map.h"
#include <map>
#include "mset.h"
#include "XmlRpcCollection.h"
#include "Serializable.h"
#include "Iterator.h"
#include "MaceTypes.h"
#include "RandomUtil.h"
#include "StrUtil.h"
#include "Traits.h"

#ifndef _NEIGHBOR_SET_H
#define _NEIGHBOR_SET_H

/**
 * \file NodeCollection.h
 * \brief defines the mace::NodeCollection class
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/**
 * \brief a hybrid set/map containing a collection of objects about nodes.
 *
 * A node collection is templated on a node type, which is a class that
 * contains a reference to the node.  It is sorted on the MaceKey referring to
 * the node, but contains a variety of other elements.
 *
 * It is like a set because it is a collection of objects, but like a map in
 * that each object has an associated (and internal) key, the node MaceKey.
 *
 * In its implementation, it maintains a NodeSet to make several of the calls
 * more efficient, and is backed by a std::map because it is more convenient.
 * The basic iterator methods return a std::map-like iterator from the base
 * class, though there is also a method to return an std::set-like iterator
 * setBegin(), setEnd(), and setFind().  Finally, for the Java fans among us,
 * it includes a mapIterator() method which returns a MapIterator iterator.
 *
 * NodeCollections also contain a MaxSize (which defaults to UINT_MAX)
 */
template <class NodeType, uint32_t MaxSize = UINT_MAX>
class NodeCollection : private std::map<MaceKey, NodeType>, public Serializable, public PrintPrintable
{
public: 
  typedef std::map<MaceKey, NodeType> mapType;
  typedef typename mapType::iterator iterator;
  typedef typename mapType::const_iterator const_iterator;
  typedef typename mapType::reverse_iterator reverse_iterator;
  typedef typename mapType::const_reverse_iterator const_reverse_iterator;
  typedef typename mapType::size_type size_type;
  typedef MapIterator<NodeCollection<NodeType, MaxSize> > map_iterator;
  typedef ConstMapIterator<NodeCollection<NodeType, MaxSize> > const_map_iterator;
  typedef NodeType mapped_type;

private: 
  //These are the things which map_iterator needs.
  friend class MapIterator<NodeCollection<NodeType, MaxSize> >;
  friend class ConstMapIterator<NodeCollection<NodeType, MaxSize> >;
    
  friend int XmlRpcMapDeserialize<NodeCollection<NodeType, MaxSize>, MaceKey, NodeType>(std:: istream& in, NodeCollection<NodeType, MaxSize>& obj) throw(SerializationException);
    
  //     using mapType::erase;
  void erase(iterator i) { myNodeSet.erase(i->first); mapType::erase(i); }
  //     iterator begin() { return mapType::begin(); }
  //     const_iterator begin() const { return mapType::begin(); }
  //     iterator end() { return mapType::end(); }
  //     const_iterator end() const { return mapType::end(); }
    
  NodeSet myNodeSet;

public:
  using mapType::begin;
  using mapType::rbegin;
  using mapType::end;
  using mapType::rend;
  using mapType::find;
  using mapType::lower_bound;
  using mapType::upper_bound;

  /// returns the maximum number elements in the collection
  inline size_t maxSize() const {
    return MaxSize;
  }

  /// lower case version of maxSize()
  inline size_t maxsize() const {
    return MaxSize;
  }

  NodeCollection() : mapType(), myNodeSet() { }
  virtual ~NodeCollection() {}

  /// returns true if the collection contains an element referring to \c who
  bool contains(const MaceKey& who) const {
    //       return mapType::find(who) != end();
    return myNodeSet.contains(who);
  }
  /// equivalent to contains(who)
  bool containsKey(const MaceKey& who) const {
    return this->contains(who);
  }

  using mapType::size;
  using mapType::empty;

  /// get the element associated with \c who, aborting if the element is not in the case
  NodeType& get(const MaceKey& who) {
    iterator i = mapType::find(who);
    if (i == end()) {
      ABORT("element not in collection");
    }
    return i->second;
  }

  /// get the element associated with \c who, aborting if the element is not in the case
  const NodeType& get(const MaceKey& who) const {
    const_iterator i = mapType::find(who);
    if (i == end()) {
      ASSERT(false);
    }
    return i->second;
  }

  /// add a new element for node \c who, returning a reference to the added element.  (returns an existing element if found)
  NodeType& add(const MaceKey& who) {
    iterator i = mapType::find(who);
    if (i == end() && size() < MaxSize) {
      myNodeSet.insert(who);
      return (this->operator[](who) = NodeType(who));
    }
    else if (i == end()) {
      ASSERT(false && size() < MaxSize);
      return (i->second); //KW: Appease the intel compiler.
    }
    else {
      return (i->second);
    }
  }

  /// adds the element \c nbr to the collection.  If already in the collection, updates the value to \c nbr
  NodeType& add(const NodeType& nbr) {
    iterator i = mapType::find(nbr.getId());
    if (i == end() && size() < MaxSize) {
      myNodeSet.insert(nbr.getId());
      return (this->operator[](nbr.getId()) = nbr);
    }
    else if (i == end()) {
      ASSERT(false && size() < MaxSize); // die
      return (i->second = nbr); //KW: Appease the intel compiler.
    }
    else {
      return (i->second = nbr);
    }
  }

  /// removes all elements from the collection
  void clear() {
    //This stub here so we can add stuff when a clear is called.
    if(!myNodeSet.empty()) {
      myNodeSet.clear();
      mapType::clear();
    }
  }

  /// erases the element with node \c who
  size_type erase(const MaceKey& who) {
    //This stub here so we can add magic stuff.
    myNodeSet.erase(who);
    return mapType::erase(who);
  }

  /// erases the element corresponding to the node \c who is associated with
  size_type erase(const NodeType& who) {
    myNodeSet.erase(who.getId());
    return mapType::erase(who.getId());
  }

  /**
   * \brief checks to see if a function result of an element matches the value \c v
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which returns something that can be compared here.
   *
   * So if there is a function NodeType::getBlockCount(), you could call
   * query(getBlockCount, 5), and it would return true if some element's
   * getBlockCount method returned the value 5.
   *
   * @param val a function of the NodeType which returns type S
   * @param v a value of type S
   * @return true if a matching element is found
   */
  template<typename S>
  bool query(S (NodeType::*val)(void), S v) {
    for(iterator i=begin(); i!=end(); i++) {
      if((i->second.*val)() == v) {
	return true;
      }
    }
    return false;
  }

  /**
   * \brief returns an iterator to end() or an element which matches the value \c v
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which returns something that can be compared here.
   *
   * So if there is a function NodeType::getBlockCount(), you could call
   * query(&NodeType::getBlockCount, 5), and it would return an iterator to
   * some element whose getBlockCount method returned the value 5.
   *
   * @param val a function of the NodeType which returns type S
   * @param v a value of type S
   * @return true if a matching element is found
   */
  template<typename S>
  iterator find(S (NodeType::*val)(void), S v) {
    for(iterator i=begin(); i!=end(); i++) {
      if((i->second.*val)() == v) {
	return i;
      }
    }
    return end();
  }

  /**
   * \brief returns an iterator to end() or an element which matches the value \c v
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which returns something that can be compared here.
   *
   * So if there is a function NodeType::getBlockCount(), you could call
   * query(&NodeType::getBlockCount, 5), and it would return an iterator to
   * some element whose getBlockCount method returned the value 5.
   *
   * @param val a function of the NodeType which returns type S
   * @param v a value of type S
   * @return true if a matching element is found
   */
  template<typename S>
  const_iterator find(S (NodeType::*val)(void), S v) const {
    for(iterator i=begin(); i!=end(); i++) {
      if((i->second.*val)() == v) {
	return i;
      }
    }
    return end();
  }

  /**
   * \brief returns the element with the lowest score (based on the score function)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the smallest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the lowest score node
   */
  template<typename ScoreType>
  NodeType& leastScore(ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType bestScore = ScoreType();
    NodeType *best_entry = NULL;
    ASSERT(!empty());
    bestScore = (begin()->second.*sc)();
    best_entry = &(begin()->second);
    //       printf("In leastScore\n");
    //       fflush(NULL);
    for(iterator i=begin(); i!=end(); i++) {
      //         printf("Considering %.8x %f\n", i->first, (i->second.*sc)());
      //         fflush(NULL);
      if ((i->second.*sc)() < bestScore) {
	//           printf("picking better score!\n");
	//           fflush(NULL);
	bestScore = (i->second.*sc)();
	best_entry = &(i->second);
      }
    }
    //       printf("returning %x\n", best_entry);
    //       fflush(NULL);
    return *best_entry;		
  }

  /**
   * \brief returns the element with the lowest score (based on the score function and provided comparitor)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the smallest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param c the comparitor to use to compare scores
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the lowest score node
   */
  template<typename ScoreType, typename Compare>
  NodeType& leastScore(const Compare& c, ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType bestScore = ScoreType();
    NodeType *best_entry = NULL;
    ASSERT(!empty());
    bestScore = (begin()->second.*sc)();
    best_entry = &(begin()->second);
    //       printf("In leastScore\n");
    //       fflush(NULL);
    for(iterator i=begin(); i!=end(); i++) {
      //         printf("Considering %.8x %f\n", i->first, (i->second.*sc)());
      //         fflush(NULL);
      if (c((i->second.*sc)(),bestScore)) {
	//           printf("picking better score!\n");
	//           fflush(NULL);
	bestScore = (i->second.*sc)();
	best_entry = &(i->second);
      }
    }
    //       printf("returning %x\n", best_entry);
    //       fflush(NULL);
    return *best_entry;		
  }

  /**
   * \brief returns the element with the lowest score (based on the score function)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the smallest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the lowest score node
   */
  template<typename ScoreType>
  const NodeType& leastScore(ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) const {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType bestScore = ScoreType();
    const NodeType *best_entry = NULL;
    ASSERT(!empty());
    bestScore = (begin()->second.*sc)();
    best_entry = &(begin()->second);
    //       printf("In leastScore\n");
    //       fflush(NULL);
    for(const_iterator i=begin(); i!=end(); i++) {
      //         printf("Considering %.8x %f\n", i->first, (i->second.*sc)());
      //         fflush(NULL);
      if ((i->second.*sc)() < bestScore) {
	//           printf("picking better score!\n");
	//           fflush(NULL);
	bestScore = (i->second.*sc)();
	best_entry = &(i->second);
      }
    }
    //       printf("returning %x\n", best_entry);
    //       fflush(NULL);
    return *best_entry;		
  }

  /**
   * \brief returns the element with the lowest score (based on the score function and provided comparitor)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the smallest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param c the comparitor to use to compare scores
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the lowest score node
   */
  template<typename ScoreType, typename Compare>
  const NodeType& leastScore(const Compare& c, ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) const {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType bestScore = ScoreType();
    const NodeType *best_entry = NULL;
    ASSERT(!empty());
    bestScore = (begin()->second.*sc)();
    best_entry = &(begin()->second);
    //       printf("In leastScore\n");
    //       fflush(NULL);
    for(const_iterator i=begin(); i!=end(); i++) {
      //         printf("Considering %.8x %f\n", i->first, (i->second.*sc)());
      //         fflush(NULL);
      if (c((i->second.*sc)(), bestScore)) {
	//           printf("picking better score!\n");
	//           fflush(NULL);
	bestScore = (i->second.*sc)();
	best_entry = &(i->second);
      }
    }
    //       printf("returning %x\n", best_entry);
    //       fflush(NULL);
    return *best_entry;		
  }

  /**
   * \brief returns the element with the greatest score (based on the score function)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the greatest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the greatest score node
   */
  template<typename ScoreType>
  NodeType& greatestScore(ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType worstScore = ScoreType();
    NodeType *worst_entry = NULL;

    ASSERT(!empty());
    worstScore = (begin()->second.*sc)();
    worst_entry = &(begin()->second);

    for(iterator i=begin(); i!=end(); i++) {
      if ((i->second.*sc)() > worstScore)
      {
	worstScore = (i->second.*sc)();
	worst_entry = &(i->second);
      }
    }
    return *worst_entry;		
  }

  /**
   * \brief returns the element with the greatest score (based on the score function and comparitor)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the greatest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param c the comparitor to use to compare scores
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the greatest score node
   */
  template<typename ScoreType, typename Compare>
  NodeType& greatestScore(const Compare& c, ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType worstScore = ScoreType();
    NodeType *worst_entry = NULL;

    ASSERT(!empty());
    worstScore = (begin()->second.*sc)();
    worst_entry = &(begin()->second);

    for(iterator i=begin(); i!=end(); i++) {
      if (c(worstScore, (i->second.*sc)()))
      {
	worstScore = (i->second.*sc)();
	worst_entry = &(i->second);
      }
    }
    return *worst_entry;		
  }

  /**
   * \brief returns the element with the greatest score (based on the score function)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the greatest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the greatest score node
   */
  template<typename ScoreType>
  const NodeType& greatestScore(ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) const {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType worstScore = ScoreType();
    const NodeType *worst_entry = NULL;

    ASSERT(!empty());
    worstScore = (begin()->second.*sc)();
    worst_entry = &(begin()->second);

    for(const_iterator i=begin(); i!=end(); i++) {
      if ((i->second.*sc)() > worstScore)
      {
	worstScore = (i->second.*sc)();
	worst_entry = &(i->second);
      }
    }
    return *worst_entry;		
  }

  /**
   * \brief returns the element with the greatest score (based on the score function and comparitor)
   *
   * \warning must be called on a non-empty collection
   *
   * Calls the sc function for each node, and returns a reference to the node with the greatest score.
   *
   * Auto-generated node types have auto-generated get methods which can be
   * passed to the query function.  Additionally, they have a getScore function
   * which by default is compared here.
   *
   * @param c the comparitor to use to compare scores
   * @param sc the score function to use, defaults to &NodeType::getScore
   * @return a reference to the greatest score node
   */
  template<typename ScoreType, typename Compare>
  const NodeType& greatestScore(const Compare& c, ScoreType (NodeType::*sc)(void) const = &NodeType::getScore) const {
    if(!mace::KeyTraits<ScoreType>::isDeterministic()) {
      return random();
    }
    ScoreType worstScore = ScoreType();
    const NodeType *worst_entry = NULL;

    ASSERT(!empty());
    worstScore = (begin()->second.*sc)();
    worst_entry = &(begin()->second);

    for(const_iterator i=begin(); i!=end(); i++) {
      if (c(worstScore, (i->second.*sc)()))
      {
	worstScore = (i->second.*sc)();
	worst_entry = &(i->second);
      }
    }
    return *worst_entry;		
  }

  /// Return a random element reference
  /// \warning must not be empty
  NodeType& random() {
    ASSERT(!empty());
    size_type i = RandomUtil::randInt(size());
    ASSERT(i < size());
    iterator j;
    for(j=begin(); j!=end() && i > 0; j++, i--);
    if(j != end()) {
      return j->second;
    } else {
      ASSERT(false);
    }
  }

  /// Return a random element const reference
  /// \warning must not be empty
  const NodeType& random() const {
    ASSERT(!empty());
    size_type i = RandomUtil::randInt(size());
    ASSERT(i < size());
    const_iterator j;
    for(j=begin(); j!=end() && i > 0; j++, i--);
    if(j != end()) {
      return j->second;
    } else {
      ASSERT(false);
    }
  }

  /// returns a node set reference of the nodes represented in the collection
  const NodeSet& nodeSet() const {
    return myNodeSet;
  }

  /// return true if the collection has room for more elements
  bool space() const {
    return size() < MaxSize;
  }

  /// return true if the collection is full
  bool full() const {
    return size() >= MaxSize;
  }

//   iterator begin() {
//     return begin();
//   }

//   const_iterator begin() const {
//     return begin();
//   }

//   iterator end() {
//     return end();
//   }

//   const_iterator end() const {
//     return end();
//   }

  /// return a map iterator of the collection, with optional start and end MaceKey objects
  map_iterator mapIterator(const MaceKey* b = NULL, const MaceKey* e = NULL) {
    return map_iterator(*this, (b==NULL?begin():mapType::find(*b)), (e==NULL?end():mapType::find(*e)));
  }
  /// return a map iterator of the collection, with optional start and end MaceKey objects
  const_map_iterator mapIterator(const MaceKey* b = NULL, const MaceKey* e = NULL) const {
    return const_map_iterator(*this, (b==NULL?begin():mapType::find(*b)), (e==NULL?end():mapType::find(*e)));
  }

  //     MapIterator<MaceKey, NodeType> iterator() {
  //       return MapIterator<MaceKey, NodeType>((map<MaceKey, NodeType>)*this);
  //     }

  void serialize(std::string& str) const {
    uint32_t s = size();
    mace::serialize(str, &s);
    for(const_iterator i=mapType::begin(); i!=mapType::end(); i++) {
      mace::serialize(str, &(i->second));
    }
  }

  int deserialize(std::istream& in) throw(SerializationException) {
    clear();
    int position = 0;
    int ret = 0;

    uint32_t s;

    ret = mace::deserialize(in, &s);
    if (ret < 0) { return -1; }
    else { position += ret; }
    ASSERT(s <= MaxSize);

    MaceKey m;

    for(size_t i=0; i<s; i++) {
      //         NodeType t;
      //         ret = mace::deserialize (in, &t, t);
      int pos = in.tellg();
      ret = mace::deserialize(in, &m);
      if (ret < 0) { return -1; }
      in.seekg(pos, std::ios::beg);
      NodeType& t = add(m);
      ret = mace::deserialize(in, &t);
      if (ret < 0) { return -1; }
    }
    return position;
  }
  
  void sqlize(LogNode* node) const {
    int index = node->nextIndex();
    std::string keyTable = node->logName + "_keys";
    std::string valueTable = node->logName + "_values";
    
    if (index == 0) {
      std::ostringstream out;
      out << "CREATE TABLE " << node->logName
	  << " (_id INT, keys INT, values INT, PRIMARY KEY(_id, keys));"
	  << std::endl;
      Log::logSqlCreate(out.str(), node);
      
      node->children = new LogNode*[2];
      LogNode* child0 = new LogNode(keyTable, Log::sqlEventsLog);
//       child0->out.open((keyTable + ".log").c_str());
      node->children[0] = child0;
      LogNode* child1 = new LogNode(valueTable, Log::sqlEventsLog);
//       child1->out.open((valueTable + ".log").c_str());
      node->children[1] = child1;
    }    
    for(const_iterator i = mapType::begin(); i != mapType::end(); i++) {
      int keyIndex = node->children[0]->nextId;
      int valueIndex = node->children[1]->nextId;
      std::ostringstream out;
      
      out << node->logId << "\t" << index << "\t" << keyIndex << "\t" 
	  << valueIndex << std::endl;
      ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
      mace::sqlize(&(i->first), node->children[0]);
      mace::sqlize(&(i->second), node->children[1]);
    }
  }
  
  void serializeXML_RPC(std::string& str) const throw(SerializationException) {
    XmlRpcMapSerialize<const_iterator, MaceKey>(str, begin(), end());
  }

  int deserializeXML_RPC(std::istream& in) throw(SerializationException) {    
    return XmlRpcMapDeserialize<NodeCollection<NodeType, MaxSize>, MaceKey, NodeType>(in, *this);
  }

  /// returns the actual type parameters as strings at runtime
  const std::string& getTypeName() const {
    const char* types[] = { "NodeType", "unsigned int MaxSize", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    return myTypes[0];
  }
  
  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "NodeCollection<" << this->getTypeName() << "," << MaxSize << "> [";
      out << " cursize=" << size() << "]";
    }
    printList(out, setBegin(), setEnd());
  }

  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "NodeCollection<" << this->getTypeName() << "," << MaxSize << "> [";
      out << " cursize=" << size() << "]";
    }
    printListState(out, setBegin(), setEnd());
  }

  class set_iterator {
  private: 
    iterator iter;
    friend class const_set_iterator;
  public:
    //         set_iterator(NodeCollection<NodeType, MaxSize>::iterator i) : iter(i) { }
    set_iterator(iterator i) : iter(i) { }
    set_iterator(const set_iterator& i) : iter(i.iter) { }
    NodeType& operator*() { return iter->second; }
    set_iterator& operator++() { iter++; return *this; }
    void operator++(int) { iter++; }
    bool operator==(const set_iterator& right) { return iter == right.iter; }
    bool operator!=(const set_iterator& right) { return iter != right.iter; }
  };

  class const_set_iterator {
  private: 
    const_iterator iter;

  public:
    const_set_iterator(const_iterator i) : iter(i) { }
    const_set_iterator(const const_set_iterator& i) : iter(i.iter) { }
    const_set_iterator(const set_iterator& i) : iter(i.iter) { }
    const NodeType& operator*() { return iter->second; }
    const_set_iterator& operator++() { iter++; return *this; }
    void operator++(int) { iter++; }
    bool operator==(const const_set_iterator& right) { return iter == right.iter; }
    bool operator!=(const const_set_iterator& right) { return iter != right.iter; }
  };

  /// a set_iterator version of begin()
  set_iterator setBegin() {
    return set_iterator(begin());
  }
  /// a set_iterator version of begin()
  const_set_iterator setBegin() const {
    return const_set_iterator(begin());
  }
  /// a set_iterator version of end()
  set_iterator setEnd() {
    return set_iterator(end());
  }
  /// a set_iterator version of end()
  const_set_iterator setEnd() const {
    return const_set_iterator(end());
  }
  /// a set_iterator version of find()
  set_iterator setFind(const MaceKey& item) {
    return set_iterator(this->find(item));
  }
  /// a set_iterator version of find()
  const_set_iterator setFind(const MaceKey& item) const {
    return set_iterator(this->find(item));
  }
};

/** @} */

} // namespace mace

#endif // _NEIGHBOR_SET_H
