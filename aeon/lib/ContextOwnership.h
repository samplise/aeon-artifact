#ifndef _CONTEXTOWNERSHIP_H
#define _CONTEXTOWNERSHIP_H

// including headers
#include "mace.h"
#include "m_map.h"
#include "ContextBaseClass.h"
// uses snapshot by default

class ContextStructureNode {
public:
  typedef std::vector< ContextStructureNode* > ContextStructureNodesType;

  ContextStructureNode(): parent_contexts( ), child_contexts( ), context_name(""), alias(""), contextId(0), isUpperBoundContextFlag(false), 
    upper_bound_ctx_name(""), version(1) { }
  ContextStructureNode(mace::string const& ctx_name, mace::string const& alias, const uint32_t ctxId, bool isUpperBoundContext = false ): 
    parent_contexts(), child_contexts(), context_name(ctx_name), alias(alias), contextId(ctxId), isUpperBoundContextFlag(isUpperBoundContextFlag), 
    upper_bound_ctx_name(""), version(1) { } 

  ~ContextStructureNode() {
    for( uint32_t i=0; i<parent_contexts.size(); i++ ) {
      parent_contexts[i] = NULL;
    }

    for( uint32_t i=0; i<child_contexts.size(); i++ ) {
      child_contexts[i] = NULL;
    }
    parent_contexts.clear();
    child_contexts.clear();
  }

  void setIsUpperBoundContext(bool flag) {
    isUpperBoundContextFlag = flag;
  }

  bool isUpperBoundContext() const { return isUpperBoundContextFlag; }

  void setParents(ContextStructureNodesType const& parents) {
    parent_contexts = parents;
  }

  void setChildren(ContextStructureNodesType const& childs) {
    child_contexts = childs;
  }

  void addParents(ContextStructureNodesType const& parents) {
    for(uint32_t i=0; i<parents.size(); i++) {
      ContextStructureNode* n_iter = parents[i];
      if( !findParent(n_iter)) {
        parent_contexts.push_back(n_iter);
      }
    }
  }

  void addChilds(ContextStructureNodesType const& childs) {
    for(uint32_t i=0; i<childs.size(); i++) {
      ContextStructureNode* n_iter = childs[i];
      if( !findChild(n_iter)) {
        child_contexts.push_back(n_iter);
      }
    }
  }

  void addParent(ContextStructureNode * parent) {
    if( !findParent(parent)) {
        parent_contexts.push_back(parent);
    }
  }

  void addChild(ContextStructureNode* child) {
    ADD_SELECTORS("ContextStructureNode::addChild");
    // macedbg(1) << "children number: " << child_contexts.size() << Log::end;
    if( !findChild(child)) {
        child_contexts.push_back(child);
        // macedbg(1) << "Add child(" << child->getCtxName() << ") for " << context_name <<", total children: " << child_contexts.size() << Log::endl;
    }
    // macedbg(1) << "context("<< this->getCtxName() <<") children number: " << child_contexts.size() << Log::endl;
  }

  void removeParent(ContextStructureNode * parent) {
    for(std::vector<ContextStructureNode*>::iterator iter = parent_contexts.begin(); iter != parent_contexts.end(); iter ++ ) {
      if( *iter == parent ) {
        parent_contexts.erase(iter);
        break;
      }
    }
  }

  void removeChild(ContextStructureNode * child) {
    for(std::vector<ContextStructureNode*>::iterator iter = child_contexts.begin(); iter != child_contexts.end(); iter ++ ) {
      if( *iter == child ) {
        child_contexts.erase(iter);
        break;
      }
    }
  }

  void clearOwnership() {
    for(std::vector<ContextStructureNode*>::iterator iter = parent_contexts.begin(); iter != parent_contexts.end(); iter ++ ) {
      (*iter)->removeChild(this);
    }

    for(std::vector<ContextStructureNode*>::iterator iter = child_contexts.begin(); iter != child_contexts.end(); iter ++ ) {
      (*iter)->removeParent(this);
    }
  }

  mace::vector<mace::string> getParentNames() const;
  mace::vector<mace::string> getChildrenNames() const;
  mace::set<mace::string> getDecendantNames();

  ContextStructureNodesType getParents() { return parent_contexts; }
  ContextStructureNodesType getChildren() const { return child_contexts; }

  const mace::string getCtxName() const { return context_name; }
  const mace::string getUpperBoundContextName() const { return upper_bound_ctx_name; }
  void getDominateContexts( mace::set<mace::string>& dominateContexts ) const;

  void getAllContextDominators(mace::map<mace::string, mace::string>& dominators) const;
  void getAllParentContextDominators(mace::string const& pDominator, mace::map<mace::string, mace::string>& dominators) const;
  void getDominateRegionContexts( const mace::string& dominator, mace::map<mace::string, mace::set<mace::string> > & dominateContexts ) const;

  void setUpperBoundContextName(mace::string const& start_ctx_name) { this->upper_bound_ctx_name = start_ctx_name; }

  bool findParent(const ContextStructureNode* node) {
    bool flag = false;
    for(uint32_t i=0; i< this->parent_contexts.size(); i++) {
      const ContextStructureNode* n_iter = this->parent_contexts[i];
      if(node->getCtxName() == n_iter->getCtxName()) {
        flag = true;
        break;
      }
    }
    return flag;
  }

  bool findChild(const ContextStructureNode* node) {
    bool flag = false;
    for(uint32_t i=0; i<child_contexts.size(); i++) {
      const ContextStructureNode* n_iter = child_contexts[i];
      if(node->getCtxName() == n_iter->getCtxName()) {
        flag = true;
        break;
      }
    }
    return flag;
  }

  bool connectToNode( const mace::string& ctx_name ) const;
  void increaseVersion();
  uint64_t getVersion() const { return version; }
  void setVersion( const uint64_t& ver );
  mace::string getRootContextName() const;
  
  static void setParentChildRelation(ContextStructureNode* p, ContextStructureNode* c) {
    ADD_SELECTORS("ContextStructure::setParentChildRelation");
    macedbg(1) << "Create Parent-Child relationship between " << p->getCtxName() << " and " << c->getCtxName() << Log::endl;
    p->addChild(c);
    c->addParent(p);
  }

  static void deleteParentChildRelation(ContextStructureNode* p, ContextStructureNode* c) {
    ADD_SELECTORS("ContextStructure::deleteParentChildRelation");
    if( p == NULL || c == NULL ) {
      return;
    }
    macedbg(1) << "Remove Parent-Child relationship between " << p->getCtxName() << " and " << c->getCtxName() << Log::endl;
    p->removeChild(c);
    c->removeParent(p);
  }

  ContextStructureNodesType parent_contexts;
  ContextStructureNodesType child_contexts;
  
private:
  mace::string context_name;
  mace::string alias;
  uint32_t contextId;
  bool isUpperBoundContextFlag;
  mace::string upper_bound_ctx_name;
  uint64_t version;
};

class ContextStructure {
public:
  static const uint8_t READER = 0;
  static const uint8_t WRITER = 1;
public:
  ContextStructure(): hasCheckUpperBoundContexts(false), rootNode(NULL), singleRootNodeMap( ), contextOwnerships( ), current_version(0), nameNodeMap( ), nreader(0), nwriter(0) {
    ASSERT( pthread_mutex_init(&contextStructureMutex, NULL) == 0);
    ASSERT( pthread_mutex_init(&updateOwnershipMutex, NULL) == 0);
  }

  ~ContextStructure() {
    pthread_mutex_destroy(&contextStructureMutex);
    pthread_mutex_destroy(&updateOwnershipMutex);

    //clearContextStructure();

    delete rootNode;
    rootNode = NULL;
  }

  mace::set<mace::string> getAllDecendantContexts(mace::string const& ctx_name) const;

  mace::set<mace::string> getOrderingDecendantContexts(mace::string const& ctx_name) const;
  mace::set<mace::string> getOrderingDecendantContexts2(mace::string const& ctx_name) const;

  mace::vector<mace::string> getAllChildContexts(mace::string const& ctx_name) const;
  mace::vector<mace::string> getAllChildContextsNoLock(mace::string const& ctx_name) const;

  mace::vector<mace::string> getAllParentContexts(mace::string const& ctx_name) const;

  mace::vector<mace::string> getDominateContexts( mace::string const& ctxName ) const;

  mace::map<mace::string, mace::set<mace::string> > getDominateRegionContexts() const;

  bool isParentContext(mace::string const& child_ctx_name, mace::string const& parent_ctx_name) const;
  bool isElderContext(mace::string const& d_ctx_name, mace::string const& elder_ctx_name) const;
  bool isUpperBoundContext(mace::string const& ctx_name) const;
  bool isUpperBoundContextNoLock(mace::string const& ctx_name) const;

  mace::string getUpperBoundContextName(mace::string const& ctx_name) const;
  mace::string getDominatorContext( mace::string const& ctxName );
  mace::string getParentDominatorContext( mace::string const& ctxName );

  void constructContextStructure();

  void printAllOwnerships() const;
  
  bool hasCheckUpperBoundContexts;

  static void setInitialContextOwnerships(const mace::vector< mace::pair<mace::string, mace::string> >& ownerships) {
    initialContextOwnerships = ownerships;
  }

  static mace::vector< mace::pair<mace::string, mace::string> > getInitialContextOwnerships() {
    return initialContextOwnerships;
  } 

  void setCurrentVersion(const uint64_t ver) { current_version = ver; }
  uint64_t getCurrentVersion() const { return current_version; }

  bool checkParentChildRelation(mace::string const& p, mace::string const& c) const;
  mace::set<mace::string> modifyOwnerships(const mace::string& dominator, const mace::vector< mace::EventOperationInfo >& ownershipOpInfos);
  void updateOwnerships(const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, const mace::map<mace::string, uint64_t>& vers);

  mace::vector< mace::pair<mace::string, mace::string> > getAllOwnerships() const;
  mace::vector< mace::pair<mace::string, mace::string> > getOwnershipOfContexts( const mace::set<mace::string>& contexts ) const;
  uint64_t getDAGNodeVersion( const mace::string& ctx_name) const;
  
  
  bool hasContextNode( mace::string const& name ) const {
    if( nameNodeMap.find(name) == nameNodeMap.end() ) {
      return false;
    } else {
      return true;
    }
  }

  bool connectToRootNode( const mace::string& ctx_name ) const;
  mace::set<mace::string> getContextsClosedToRoot( const mace::set<mace::string>& contexts ) const;
  mace::vector<mace::string> sortContexts( const mace::set<mace::string>& contexts ) const;
  
  // ownership modification
  void getLock( const uint8_t lock_type );
  void releaseLock( const uint8_t lock_type );

  mace::map<mace::string, mace::string> getAllContextDominators() const;
  mace::map<mace::string, mace::string> getAllParentContextDominators() const;

  mace::map<mace::string, mace::set<mace::string> > getBroadcastCPRelations( mace::string const& ctxName ) const;
  mace::string getParentContextName( mace::string const& ctxName ) const;
  
  mutable pthread_mutex_t contextStructureMutex;
  mutable pthread_mutex_t updateOwnershipMutex;
  
private:
  ContextStructureNode* rootNode;
  mace::map<mace::string, mace::string> singleRootNodeMap;

  static mace::vector< mace::pair<mace::string, mace::string> > initialContextOwnerships;
  
  mace::set< mace::pair<mace::string, mace::string> > contextOwnerships;
  uint64_t current_version;

  std::map<mace::string, ContextStructureNode*> nameNodeMap;

  std::deque< std::pair<uint8_t, pthread_cond_t*> > waitingConds;
  uint64_t nreader;
  uint64_t nwriter;

  void traverseAndUpdateUpperBoundContextsInForest();
  void traverseAndUpdateUpperBoundContexts( const mace::string& ctx_name );
  void traverseAndUpdateUpperBoundContexts(ContextStructureNode* node, mace::set<mace::string>& pNode_children, 
    mace::set<mace::string>& pNode_parents);
  void broadcastUpperBoundContext(ContextStructureNode* node, mace::string const& ctx_name);
  ContextStructureNode* findContextNodeNoLock(mace::string const& ctx_name);
  
  const ContextStructureNode* const_findContextNodeNoLock(mace::string const& ctx_name) const;

  bool addParentChildRelationNoLock( mace::string const& parent_ctx_name,  ContextStructureNode* child_node );
  bool addChildParentRelation(mace::string const& child_ctx_name, ContextStructureNode* parent_node);
  
  void clearUnusedContextStructureNodes();
  bool isElderContextNoLock(mace::string const& d_ctx_name, mace::string const& elder_ctx_name) const;
  void getAllOwnerships( const ContextStructureNode* node, mace::vector< mace::pair<mace::string, mace::string> >& ownerships) const;
  
  void clearUpperBoundContextFlag() {
    clearUpperBoundContextFlag(rootNode);
  }

  void clearUpperBoundContextFlag(ContextStructureNode* node);

  
};

#endif
