/* 
 * ContextMapping.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2013, Wei-Chiu Chuang
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

#ifndef CONTEXTMAPPING_H
#define CONTEXTMAPPING_H
/**
 * \file ContextMapping.h
 * \brief declares the ContextMapping class for mapping context to physical nodes.
 */

#include "MaceKey.h"
#include "m_map.h"
#include "mvector.h"
#include "mlist.h"
#include "mace-macros.h"
#include "Util.h"
#include "SockUtil.h"
#include "ThreadStructure.h"
#include "Event.h"
#include <utility>
#include <deque>
#include "Serializable.h"
#include "Printable.h"
#include "mace.h"
#include "ElasticPolicy.h"

/**
 * \file ContextMapping.h
 * \brief declares the ContextMapping class and ContextEventRecord class.
 */
namespace mace {
  /**
   * \brief Context record at head node
   *
   * */
  
  class ContextEventRecord {
  // TODO: make event record to be integer array
    class ContextNode;
    typedef mace::vector< ContextNode*, mace::SoftState > ContextNodeType;
    typedef mace::list< ContextNode*, mace::SoftState>  ChildContextNodeType;
    typedef mace::hash_map< mace::string, uint64_t > ContextNameIDType;

  public:
    ContextEventRecord(){
			OrderID firstEventID(0, 1);
      contexts.push_back(  new ContextNode("(head)", 0, firstEventID ) );
    }
    ~ContextEventRecord(){
      for( ContextNodeType::iterator cnIt = contexts.begin(); cnIt != contexts.end(); cnIt++ ){
        delete *cnIt; 
      }
    }

    /// record a new context in the head node.
    /// \todo now that contexts can be deleted, new context id will be generated differently.
    void createContextEntry( const mace::string& contextName, const uint32_t contextID, const OrderID& firstEventID ){
      ADD_SELECTORS ("ContextEventRecord::createContext");
      // TODO: mutex lock?
      ASSERTMSG( contexts.size() == contextID , "The newly added context id doesn't match the expected value!" );
      ContextNode* node = new ContextNode(contextName, contextID, firstEventID );
      contexts.push_back( node );

      contextNameToID[ contextName ] = contextID;

      if( ! isGlobalContext( contextName ) ){
        // update the parent context node
        ContextNode* parent = getParentContextNode( contextName);
        ASSERT( parent != node );
        parent->childContexts.push_back( node );
      }
    }
    
    /**
     * update the now_serving number of the context.
     * Returns the last now_serving number
     * */
    // void updateContext( const mace::string& contextName, const mace::OrderID& newEventID, 
    //     mace::Event::EventSkipRecordType& childContextSkipRecords ){
    //   ADD_SELECTORS ("ContextEventRecord::updateContext");
    //   uint32_t contextID = findContextIDByName( contextName );

    //   updateContext( contextID, newEventID, childContextSkipRecords );
    // }
    //TODO: WC: transform the recursion into iteration.
    //  void updateContext( const uint32_t contextId, const mace::OrderID& newEventId, mace::Event::EventSkipRecordType& childContextSkipRecords ){
    //   ADD_SELECTORS ("ContextEventRecord::updateContext");
      
    // }
    // void updateChildContext( ContextNode* node, const mace::OrderID& pastEventID, const mace::OrderID& newEventID, 
    //     mace::Event::EventSkipRecordType& childContextSkipRecords ){
    //   ADD_SELECTORS ("ContextEventRecord::updateChildContext");
      
    // }
    void deleteContextEntry( mace::string const& contextName, uint32_t contextID, mace::OrderID const& eventID ){
      delete contexts[ contextID ];
      contextNameToID.erase( contextName );

    }


    
  protected:
  private:
    /**
     * \brief Internal structure to store a context record 
     *
     * */
    
    class ContextNode{
    public:
      ContextNode( const mace::string& contextName, const uint32_t contextID, const OrderID& firstEventID ):
        contextName( contextName ), contextID( contextID ), current_now_serving( firstEventID){

      }
      mace::string contextName;
      uint32_t contextID;
      OrderID current_now_serving;
      ChildContextNodeType childContexts;
    };
    ContextNameIDType contextNameToID;
    ContextNodeType contexts;

    inline bool isGlobalContext(const mace::string& contextID) {
      return contextID == "globalContext";
    }

    /* internal translation from contextName (string) to contextID (integer)
     * internally everything is recorded using the contextID for efficiency. 
     * To communicate with the external interface,
     * the transation from name to id is necessary.
     * */
    inline uint32_t findContextIDByName(const mace::string& contextName){
      ContextNameIDType::iterator ctxIDIt = contextNameToID.find( contextName );
      ASSERT( ctxIDIt != contextNameToID.end() );
      return ctxIDIt->second;
    }

    ContextNode* getParentContextNode(const mace::string& childContextID){
      ADD_SELECTORS ("ContextEventRecord::getParentContextNode");
      // find the parent context
      mace::string parent;

      ASSERTMSG( childContextID != "globalContext", "global context does not have parent!" );

      size_t lastDelimiter = childContextID.find_last_of("." );
      if( lastDelimiter == mace::string::npos ){
        parent = "globalContext"; // global context
      }else{
        parent = childContextID.substr(0, lastDelimiter );
      }
      uint32_t parentID = findContextIDByName( parent );

      ASSERTMSG( parentID < contexts.size(), "Parent ID not found in contexts!"  );

      return contexts[ parentID ];

    }
  };


    class ContextMapEntry: public PrintPrintable, public Serializable{
    public:
      mace::MaceAddr addr;
      //mace::set< uint32_t > child;
      mace::string name;
      //uint32_t parent;

      ContextMapEntry() {} // default constructor required by mace::hash_map
      ContextMapEntry( const mace::MaceAddr& addr, /*const mace::set< uint32_t >& child,*/ const mace::string& name/*, const uint32_t parent*/ ):
        addr( addr ), /*child( child ),*/ name( name )/*, parent( parent )*/{
      }

      void print(std::ostream& out) const;
      void printNode(PrintNode& pr, const std::string& name) const;

      virtual void serialize(std::string& str) const{
        mace::serialize( str, &addr );
        //mace::serialize( str, &child );
        mace::serialize( str, &name );
        //mace::serialize( str, &parent );
      }
      virtual int deserialize(std::istream & is) throw (mace::SerializationException){
        int serializedByteSize = 0;
        serializedByteSize += mace::deserialize( is, &addr   );
        //serializedByteSize += mace::deserialize( is, &child   );
        serializedByteSize += mace::deserialize( is, &name   );
        //serializedByteSize += mace::deserialize( is, &parent   );
      
        return serializedByteSize;
      }
    };

    class ContextNodeInformation: public PrintPrintable, public Serializable {
    public:
      
    public:
      mace::MaceAddr nodeAddr;
      uint32_t nodeId;

    private:
      mace::map< mace::string, uint32_t > contextTypeNumberMap;
      mace::set< mace::string > contextNames;

    public:
      ContextNodeInformation(): nodeAddr(), nodeId(0) { }
      ContextNodeInformation( const mace::MaceAddr& addr, const uint32_t node_id): nodeAddr(addr), nodeId( node_id ) { }
      ~ContextNodeInformation() { }


      void print(std::ostream& out) const { };
      void printNode(PrintNode& pr, const std::string& name) const { };

      virtual void serialize(std::string& str) const;
      virtual int deserialize(std::istream & is) throw (mace::SerializationException);

      uint32_t getNContextByTypes( mace::set< mace::string> const& context_types, mace::set<mace::string> const& inactivateContexts ) const;
      bool hasContext( mace::string const& contextName ) const;
      void addContext( mace::string const& contextName );
      void removeContext( mace::string const& contextName );
      uint32_t getTotalNContext() const { return contextNames.size(); }
    };

  /** 
   * \brief maps context to node or vice versa
   *
   * Each context service has its context mapping. Whenever a new context is added, or when a context is migrated to a new node,
   * head node creates a new mapping version.
   * */
  class ContextMapping: public PrintPrintable, public Serializable
  {
  public:
    ContextMapping () : nContexts(0), current_version(0), expect_version(0) {
      // empty initialization
    }
    ContextMapping (const mace::MaceAddr & vhead, const mace::map < mace::MaceAddr, mace::list < mace::string > >&mkctxmapping) {
      ADD_SELECTORS ("ContextMapping::(constructor)");
    }
    /// override assignment operator
    ContextMapping& operator=(const ContextMapping& orig){
      ADD_SELECTORS("ContextMapping::operator=");
      ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
      head = orig.head;
      mapping = orig.mapping;
      nodes = orig.nodes;
      nContexts = orig.nContexts;
      nameIDMap = orig.nameIDMap;
			current_version = orig.current_version;
      expect_version = orig.expect_version;

      macedbg(1) << "current_version=" << current_version << Log::endl;
      return *this;
    }
    /// copy constructor
    ContextMapping (const mace::ContextMapping& orig) { 
      *this = orig ;
    }
    // destructor
    ~ContextMapping(){
      
    }
    void print(std::ostream& out) const;
    void printNode(PrintNode& pr, const std::string& name) const;
    uint64_t getCurrentVersion() const {
      if( !update_flag && latestContextMapping != NULL){
        return latestContextMapping->getLatestMappingVersion();
      }
      return current_version; 
    }
    uint64_t getLatestMappingVersion() const {
      return current_version;
    }
    uint64_t getExpectVersion() const { return expect_version; }
    void setCurrentVersion(const uint64_t ver ) { current_version = ver; }
    void setExpectVersion(const uint64_t ver) { expect_version = ver; }
    //get latest context mapping, only used during migration
    ContextMapping* getCtxMapCopy() {
      ScopedLock sl(alock);
      ContextMapping* ctxmapSnapshot = new ContextMapping(*this);
      return ctxmapSnapshot;
    }

    const ContextMapping& getLatestContextMapping() const {
      ADD_SELECTORS("ContextMapping::getLatestContextMapping");
      ScopedLock sl(alock);
      // macedbg(1) << "current_version=" << current_version << "; last_version=" << last_version << Log::endl;
      if( latestContextMapping == NULL ) {
        //sl.unlock();
        latestContextMapping = new ContextMapping(*this);
      } else if( current_version > last_version && update_flag ) {
        macedbg(1) << "current_version=" << current_version << "; last_version=" << last_version << Log::endl;
        versionMap[last_version] = latestContextMapping;
        macedbg(1) << "Update version from " << last_version << " to " << current_version << Log::endl;
        last_version = current_version;
        latestContextMapping = new ContextMapping(*this);
        macedbg(1) << "After Update latestContextMapping version=" << latestContextMapping->getCurrentVersion() << Log::endl;
      }

      return *latestContextMapping;
    }

    void updateToNewMapping( const ContextMapping& orig ){
      ADD_SELECTORS("ContextMapping::updateToNewMapping");
      if( this == &orig ){
        return;
      }
      ScopedLock sl(alock);
      head = orig.head;
      mapping = orig.mapping;
      nodes = orig.nodes;
      nContexts = orig.nContexts;
      nameIDMap = orig.nameIDMap;
      current_version = orig.current_version;
      macedbg(1) << "current_version=" << current_version << Log::endl;
      expect_version = orig.expect_version;
    }

    void setUpdateFlag(const bool new_flag) {
      ScopedLock sl(alock);
      update_flag = new_flag;
    }

    /*********************************** Elasticity ***********************************************/
    void colocateContexts( mace::string const& ctx_name1, mace::string const& ctx_name2 );
    void separateContexts( mace::string const& ctx_name1, mace::string const& ctx_name2 );
    void isolateContext( mace::string const& ctx_name );
    void inactivateContext( const mace::string& ctx_name );
    void activateContext( const mace::string& ctx_name );

    virtual void serialize(std::string& str) const{
        mace::serialize( str, &head );
        mace::serialize( str, &mapping );
        mace::serialize( str, &nodes );
        mace::serialize( str, &nContexts );
        mace::serialize( str, &nameIDMap );
				mace::serialize( str, &current_version );
    }
    virtual int deserialize(std::istream & is) throw (mace::SerializationException){
        int serializedByteSize = 0;

        serializedByteSize += mace::deserialize( is, &head   );
        serializedByteSize += mace::deserialize( is, &mapping   );
        serializedByteSize += mace::deserialize( is, &nodes   );
        serializedByteSize += mace::deserialize( is, &nContexts   );
        serializedByteSize += mace::deserialize( is, &nameIDMap   );
				serializedByteSize += mace::deserialize( is, &current_version );

        return serializedByteSize;
    }
    void setDefaultAddress (const MaceAddr & addr) {
      ScopedLock sl (alock);
      head = addr;
    }
    /// public interface of snapshot()
    const mace::ContextMapping* snapshot(const uint64_t ver) const{
        mace::ContextMapping* _ctx = new mace::ContextMapping(*this); // make a copy
        snapshot( ver, _ctx );
        ThreadStructure::setEventContextMappingVersion(ver);

        return _ctx;
    }
    /// create a read-only snapshot using the current event ticket as the version number
    /// @return mace::ContextMapping* the created snapshot object
    const mace::ContextMapping* snapshot() const{
			const mace::Event& e = ThreadStructure::myEvent();
      return snapshot( e.eventContextMappingVersion );
    }
    /// returns true if there is a snapshot with version equals current event ticket number
    bool hasSnapshot() const{
			const mace::Event& e = ThreadStructure::myEvent();
      return hasSnapshot( e.eventContextMappingVersion );
    }
    /// returns true if there is a snapshot with version equals ver
    /// @param ver the snapshot version number
    bool hasSnapshot(const uint64_t ver) const;
    /// insert a context mapping snapshot.
    void snapshotInsert(const uint64_t ver, const mace::ContextMapping& snapshotMap) const{
        mace::ContextMapping* _ctx = new mace::ContextMapping( snapshotMap ); // make a copy
        snapshot( ver, _ctx );
    }
    /// remove a context mapping snapshot.
    void snapshotRelease(const uint64_t ver) const{ // clean up when event commits
      ADD_SELECTORS("ContextMapping::snapshotRelease");
      ScopedLock sl( alock );
      while( !versionMap.empty() && versionMap.begin()->first < ver ){
        macedbg(1) << "Deleting snapshot version " << versionMap.begin()->first << " for service " << this << " value " << versionMap.begin()->second << Log::endl;
        delete versionMap.begin()->second;
        versionMap.erase( versionMap.begin() );
      }
    }

    void loadMapping (const mace::map < mace::MaceAddr, mace::list < mace::string > >&mkctxmapping) {
      ScopedLock sl (alock);
      ADD_SELECTORS ("ContextMapping::loadMapping");
      for (mace::map < mace::MaceAddr, mace::list < mace::string > >::const_iterator mit = mkctxmapping.begin (); mit != mkctxmapping.end (); mit++) {
        for (mace::list < mace::string >::const_iterator lit = mit->second.begin (); lit != mit->second.end (); lit++) {
          if (lit->compare ( headContextName ) == 0) { // special case: head node
            head = mit->first;
            nodes[ head ] ++;
          } else {
            defaultMapping[ *lit ] = mit->first;
          }
        }
      }
    }

    bool checkMappingVersion( const uint64_t ver ) const {
      ADD_SELECTORS("ContextMapping::checkMappingVersion");
      macedbg(1) << "Thread identifier: " << ThreadStructure::getEventContextMappingVersion() << Log::endl; 
      VersionContextMap::const_iterator i = versionMap.find( ver );
      if (i == versionMap.end()) {
        macedbg(1) << "Fail to find mapping version: " << ver << Log::endl;
        return false;
      } else {
        macedbg(1) << "Find mapping version: " << ver << Log::endl;
        return true;
      }
    }
    /// return a snapshot context mapping object
    const mace::ContextMapping& getSnapshot(const uint64_t lastWrite) const{
      // assuming the caller of this method applies a mutex.
      ADD_SELECTORS ("ContextMapping::getSnapshot");
      ScopedLock sl (alock);

      VersionContextMap::const_iterator i = versionMap.find( lastWrite );
      if (i == versionMap.end()) {
        // TODO: perhaps the context mapping has not arrived yet.
        // block waiting
        pthread_cond_t cond;
        pthread_cond_init( &cond, NULL );
        snapshotWaitingThreads[ lastWrite ].insert( &cond );
        macedbg(1)<< "The context map snapshot version "<< lastWrite <<" has not arrived yet. wait for it"<< Log::endl;
        pthread_cond_wait( &cond, &alock );
        pthread_cond_destroy( &cond );
        i = versionMap.find( lastWrite );
        ASSERT( i != versionMap.end() );
      } else {
        //macedbg(1) << "Get the mapping version: " << lastWrite << Log::endl;
      }
      sl.unlock();
      macedbg(1)<<"Read from snapshot version: "<< lastWrite <<Log::endl;
      return *(i->second);
    }
    /// return a snapshot context mapping object of version equals the current event ticket
    const mace::ContextMapping& getSnapshot() const{
      // assuming the caller of this method applies a mutex.
      ADD_SELECTORS ("ContextMapping::getSnapshot");
      //const uint64_t lastWrite = ThreadStructure::getEventContextMappingVersion();
      const uint64_t lastWrite = current_version;
      //checkMappingVersion(lastWrite);
      return getSnapshot( lastWrite );
    }
    /// find in the given snapshot object, the node corresponding to a context, using the canonical name of the context
    static const mace::MaceAddr& getNodeByContext (const mace::ContextMapping& snapshotMapping, const mace::string & contextName)
    {
      const uint32_t contextID = snapshotMapping.findIDByName( contextName );
      return snapshotMapping._getNodeByContext( contextID );
    }
    /// find in the given snapshot object, the node corresponding to a context, using the numberical id of the context
    static const mace::MaceAddr& getNodeByContext (const mace::ContextMapping& snapshotMapping, const uint32_t contextID)
    {
      return snapshotMapping._getNodeByContext( contextID );
    }
    const mace::MaceAddr& getNodeByContext (const mace::string & contextName) const
    {
      const mace::ContextMapping& ctxmapSnapshot = getSnapshot();
      const uint32_t contextID = ctxmapSnapshot.findIDByName( contextName );
      return ctxmapSnapshot._getNodeByContext( contextID );
    }
    const bool hasContext (const mace::string & contextName) const
    {
      const mace::ContextMapping& ctxmapSnapshot = getLatestContextMapping();
      return ctxmapSnapshot._hasContext( contextName );
    }
    const bool hasContext ( uint64_t const& version, const mace::string & contextName) const
    {
      const mace::ContextMapping& ctxmapSnapshot = getSnapshot( version );
      return ctxmapSnapshot._hasContext( contextName );
    }

    /* this is another version of findIDByName, but upon unknown context name, instead of abort, it returns zero 
     * */
    static uint32_t hasContext2 ( const mace::ContextMapping& snapshotMapping, const mace::string & contextName)
    {
      mace::hash_map< mace::string, uint32_t >::const_iterator mit = snapshotMapping.nameIDMap.find( contextName );
      if( mit == snapshotMapping.nameIDMap.end() ){
        return 0;
      }
      return mit->second;
    }

    bool updateMapping (const mace::MaceAddr & oldNode, const mace::MaceAddr & newNode) {
      ADD_SELECTORS ("ContextMapping::updateMapping");

      ScopedLock sl (alock);
      uint32_t contexts = 0;
      for ( ContextMapType::iterator mit = mapping.begin (); mit != mapping.end (); mit++) {
        if (mit->second.addr == oldNode) {
            mit->second.addr = newNode;
            contexts++;
        }
      }
      // XXX: head node??
      nodes[ newNode ] = contexts;
      if( oldNode != head ){
        nodes.erase( oldNode );
      }
      // FIXME: what is newNode already exist in the logical node?
      return true;

    }
    template< class T > 
    bool updateMapping (const mace::MaceAddr & node, const T& contexts)
    {
      ADD_SELECTORS ("ContextMapping::updateMapping");

      typename T::const_iterator lit;
      for ( lit = contexts.begin (); lit != contexts.end (); lit++) {
        updateMapping( node, *lit );
      }
      return true;

    }
    std::pair< bool , uint32_t> updateMapping (const mace::MaceAddr & node, const mace::string & context);
    // create a new mapping for a context not mapped before.
    // @return a pair of the MaceAddr as well as the numbercal ID of the context
    const std::pair< mace::MaceAddr, uint32_t> newMapping( const mace::string& contextName, const mace::ElasticityRule& rule, 
      const mace::string& pContextName );

    mace::map<mace::string, mace::MaceAddr> scaleTo(const uint32_t& n);

    mace::MaceAddr getExternalCommContextNode( const mace::string& contextName ) const;

    mace::MaceAddr removeMapping( mace::string const& contextName ){
      const uint32_t contextID = findIDByName( contextName );
      nContexts --;

      mace::MaceAddr nodeAddr = _getNodeByContext( contextID );
      //mace::map < mace::MaceAddr, uint32_t >::iterator mit = mapping.find( nodeAddr );
      /*
      ContextMapType::iterator mit = mapping.find( contextID );

      ContextMapEntry & entry = mit->second;
      ASSERT( entry.child.empty() ); // must not have child context
      mapping.erase( mit );
      */
      mace::map < mace::MaceAddr, uint32_t >::iterator nodeIt = nodes.find( nodeAddr );
      if( --nodeIt->second  == 0 ){
        nodes.erase( nodeIt );
      }


      nameIDMap.erase( contextName );

      return nodeAddr;
    }
    void newHead( const MaceAddr& newHeadAddr ){
      head = newHeadAddr;
    }

    void printMapping () const
    {
      const mace::ContextMapping& ctxmapSnapshot = getSnapshot();
      return ctxmapSnapshot._printMapping(  );
    }
    
    const mace::MaceAddr & getHead () const
    {
      //ADD_SELECTORS ("ContextMapping::getHead");
      //const mace::ContextMapping& ctxmapSnapshot = getSnapshot();
      //return ctxmapSnapshot._getHead(  );
      return _getHead();
    }
    static const mace::MaceAddr & getHead (const mace::ContextMapping& ctxmapSnapshot){
      ADD_SELECTORS("ContextMapping::getHead#1");
      //macedbg(1) << "Headnode Addr: " << ctxmapSnapshot._getHead(  ) << Log::endl;
      return ctxmapSnapshot._getHead(  );
    }
    /*static void setHead (const mace::MaceAddr & h)
    {
      ADD_SELECTORS ("ContextMapping::setHead");
      ScopedLock sl (alock);
      head = h;
    }*/
    const mace::map < MaceAddr, uint32_t >& getAllNodes ()
    {
      const mace::ContextMapping& ctxmapSnapshot = getSnapshot();
      return ctxmapSnapshot._getAllNodes(  );
    }
    static const mace::map < MaceAddr, uint32_t >& getAllNodes (const mace::ContextMapping& ctxmapSnapshot)
    {
      return ctxmapSnapshot._getAllNodes(  );
    }

    // this method modifies the latest context mapping.
    static void updateVirtualNodes (const uint32_t nodeid, const MaceAddr & addr)
    {
      ScopedLock sl (alock);
      virtualNodes[nodeid] = addr;
    }

    // chuangw: not used??
    static const MaceAddr & lookupVirtualNode (const uint32_t nodeid)
    {
      ScopedLock sl (alock);
      return virtualNodes[nodeid];
    }

    // the representative name for head context does not change.
    static const mace::string& getHeadContext ()
    {
      return headContextName;
    }
    static const uint32_t getHeadContextID ()
    {
      return headContext;
    }
    
    // virtual node mace key, after assigned, does not change
    static MaceKey & getVirtualNodeMaceKey ()
    {
      return vnodeMaceKey;
    }
    static void setVirtualNodeMaceKey (const MaceKey & addr)
    {
      vnodeMaceKey = addr;
    }

    static void setInitialMapping (const mace::map < mace::string, mace::map < MaceAddr , mace::list < mace::string > > >&mapping)
    {
      initialMapping = mapping;
    }

    static void setServerAddrs( const mace::vector<MaceAddr>& addrs ) {
      ADD_SELECTORS("ContextMapping::setServerAddrs");
      serverAddrs = addrs;
      for( uint32_t i=0; i<serverAddrs.size(); i++ ){
        macedbg(1) << "Node Address: " << serverAddrs[i] << Log::endl;
        ContextNodeInformation nodeInfo( serverAddrs[i], i );
        nodeInfos[ serverAddrs[i] ] = nodeInfo;
      }
    }

    static mace::map < MaceAddr , mace::list < mace::string > >&getInitialMapping (const mace::string & serviceName)
    {
      return initialMapping[serviceName];
    }

    static mace::map < mace::string, mace::map < MaceAddr, mace::list < mace::string > > >&getInitialMapping ()
    {
      return initialMapping;
    }
    static const mace::string& getNameByID( const mace::ContextMapping& snapshotMapping, const uint32_t contextID ){
      ContextMapType::const_iterator it = snapshotMapping.mapping.find( contextID );
      ASSERT( it != snapshotMapping.mapping.end() );
      return it->second.name;
    }
    const uint32_t findIDByName( const mace::string& contextName ) const {
      ADD_SELECTORS("ContextMapping::findIDByName");
      if( contextName == "NULL" ) {
        ASSERT(false);
      }
      mace::hash_map< mace::string, uint32_t >::const_iterator mit = nameIDMap.find( contextName );
      if( mit == nameIDMap.end() ){
        ADD_SELECTORS ("ContextMapping::findIDByName");
        maceerr<<"context '" << contextName << "' is not found!"<<Log::endl;
        ABORT("context not found");
      }
      return mit->second;
    }
    
    void getContextsOfNode(mace::MaceAddr const& nodeAddr, mace::list<mace::string >& contextNames) const{  // chuangw: inefficient traversal... 

      for( ContextMapType::const_iterator mIt = mapping.begin(); mIt != mapping.end(); mIt ++ ){
        if( mIt->second.addr == nodeAddr ){
          mace::string contextName = mace::ContextMapping::getNameByID( *this, mIt->first );
          contextNames.push_back( contextName );
        }

      }
    }
    
    mace::vector<mace::MaceAddr> getNodesAddrs() {
      mace::vector<mace::MaceAddr> nodesAddrs;
      
      mace::map< mace::MaceAddr, uint32_t>::iterator iter = nodes.begin();
      for( ; iter!=nodes.end(); iter++ ){
        nodesAddrs.push_back(iter->first);
      }

      return nodesAddrs;
    }

    static const uint32_t GLOBAL_CONTEXT_ID = 1;
    static const uint32_t HEAD_CONTEXT_ID = 0;
    static const mace::string GLOBAL_CONTEXT_NAME;
    static const mace::string EXTERNAL_COMM_CONTEXT_NAME;
    static const mace::string EXTERNAL_SUB_COMM_CONTEXT_NAME;

    static const mace::string DISTRIBUTED_PLACEMENT;
  private:
    mace::string getParentContextName( const mace::string& contextID )const {
      mace::string parent;
      size_t lastDelimiter = contextID.find_last_of("." );
      if( lastDelimiter == mace::string::npos ){
        parent = ""; // global context
      }else{
        parent = contextID.substr(0, lastDelimiter );
      }
      return parent;
    }
    // add a new context entry into mapping
    void insertMapping(const mace::string& contextName, const mace::MaceAddr& addr){
      ASSERT( !_hasContext( contextName ) );
      nContexts++;
      
      ContextMapEntry entry(addr, contextName);
      mapping[ nContexts ] = entry; 

      nameIDMap[ contextName ] = nContexts;
    }
    void snapshot(const uint64_t ver, mace::ContextMapping* _ctx) const{
      ADD_SELECTORS("ContextMapping::snapshot");
      macedbg(1) << "Snapshotting version " << ver << " mapping: " << *_ctx << Log::endl;
      ScopedLock sl (alock);

      versionMap.insert( std::make_pair(ver, _ctx) );

      std::map< uint64_t, std::set< pthread_cond_t* > >::iterator condSetIt = snapshotWaitingThreads.find( ver );
      if( condSetIt != snapshotWaitingThreads.end() ){
        for( std::set< pthread_cond_t* >::iterator condIt = condSetIt->second.begin(); condIt != condSetIt->second.end(); condIt++ ){
          pthread_cond_signal( *condIt );
        }
        snapshotWaitingThreads.erase( condSetIt );

      }
    }
    const mace::MaceAddr& _getNodeByContext (const uint32_t contextID) const
    {
      ADD_SELECTORS ("ContextMapping::getNodeByContext");

      if(contextID == GLOBAL_CONTEXT_ID) {
        return getHead();
      }
      ContextMapType::const_iterator it = mapping.find( contextID );
      ASSERTMSG( it != mapping.end(), "can't find corresponding context!");
      
      return it->second.addr;
    }
    const bool _hasContext (const mace::string & contextName) const
    {
      ADD_SELECTORS ("ContextMapping::hasContext");
      return nameIDMap.count( contextName ) == 1;
    }
    
    void _printMapping () const
    {
      ADD_SELECTORS ("ContextMapping::printMapping");

      for ( ContextMapType::const_iterator mapIt = mapping.begin (); mapIt != mapping.end (); mapIt++) {
          macedbg(1) << mapIt->first <<" -> " << mapIt->second << Log::endl;
      }
    }
    const MaceAddr& _getHead () const {
      ADD_SELECTORS ("ContextMapping::_getHead");
      //macedbg(1) << "Headnode=" << head << Log::endl; 

      return head;
    }
    const mace::map < MaceAddr, uint32_t >& _getAllNodes () const
    {
      return nodes;
    }

public:
    static ContextMapping* latestContextMapping;
    static uint64_t last_version;		
    static bool update_flag;

protected:
    typedef std::map< uint64_t, const mace::ContextMapping* > VersionContextMap;
    mutable VersionContextMap versionMap;

private:
    typedef mace::hash_map < uint32_t,  ContextMapEntry> ContextMapType;

    mace::map<mace::string, mace::MaceAddr > defaultMapping; ///< User defined mapping. This should only be accessed by head node. Therefore it is not serialized
    
    mace::MaceAddr head;
    ContextMapType mapping; ///< The mapping between contexts to physical node address
    mace::map < mace::MaceAddr, uint32_t > nodes; ///< maintain a counter of contexts on this node. When it decrements to zero, remove the node from node set.
    uint32_t nContexts; ///< number of total contexts currently

    mace::hash_map< mace::string, uint32_t > nameIDMap;

		mace::map<uint64_t, mace::OrderID> verEventMap;

		uint64_t current_version;
    uint64_t expect_version;

    mace::set<mace::string> inactivateContexts;
       

    ///<------ static members
    static const uint32_t headContext;
    static mace::string headContextName;
    static pthread_mutex_t alock; ///< This mutex is used to protect static variables -- considering to drop it because process-wide sharing should use AgentLock instead.
    static std::map < uint32_t, MaceAddr > virtualNodes;
    static mace::MaceKey vnodeMaceKey; ///< The local logical node MaceKey
    static mace::map < mace::string, mace::map < MaceAddr, mace::list < mace::string > > >initialMapping;
    static std::map< uint64_t, std::set< pthread_cond_t* > > snapshotWaitingThreads;

    static uint32_t nextExternalCommNode;
    // physical machine information
    static mace::map< MaceAddr, mace::ContextNodeInformation > nodeInfos;
        
public:
    static mace::vector< MaceAddr > serverAddrs;
  };

  struct addSnapshotContextID {
    mace::ContextMapping const& currentMapping;
    mace::vector< uint32_t >& contextIDVector;
    addSnapshotContextID( mace::ContextMapping  const& currentMapping, mace::vector< uint32_t >& snapshotContextID ): 
      currentMapping(currentMapping), contextIDVector(snapshotContextID) { }

    void operator() ( mace::string const& contextIDName ){
      contextIDVector.push_back(  currentMapping.findIDByName( contextIDName ) );
    }
  };

}

#endif // CONTEXTMAPPING_H
