#include "ContextOwnership.h"
/**************************************** class ContextStructureNode *************************************************/
mace::string ContextStructureNode::getRootContextName() const {
  if( parent_contexts.size() == 0 ){
    return this->context_name;
  } else {
    return parent_contexts[0]->getRootContextName();
  }
}

void ContextStructureNode::increaseVersion() {
  ADD_SELECTORS("ContextStructureNode::increaseVersion");
  version ++;
  macedbg(1) << "context("<< this->context_name <<") DAG version increases to " << version << Log::endl;
}

void ContextStructureNode::setVersion( const uint64_t& ver ) {
  ADD_SELECTORS("ContextStructureNode::setVersion");
  macedbg(1) << "context("<< this->context_name <<") sets version to be "<< ver <<" from " << version << Log::endl;
  version = ver;
}

/**************************************** class ContextStructure *************************************************/
mace::vector< mace::pair<mace::string, mace::string> > ContextStructure::initialContextOwnerships;

const uint8_t ContextStructure::READER;
const uint8_t ContextStructure::WRITER;

bool ContextStructure::addParentChildRelationNoLock( mace::string const& parent_ctx_name,  ContextStructureNode* child_node ) {
  ADD_SELECTORS("ContextStructure::addParentChildRelation");
  ContextStructureNode* parent_node = this->findContextNodeNoLock(parent_ctx_name);
  if(parent_node == NULL) {
    return false;
  }
  //macedbg(1) << "Add child(" << child_node->getCtxName() << ") for " << parent_node->getCtxName() << Log::endl;
  parent_node->addChild(child_node);
  child_node->addParent(parent_node);
  return true;
}

bool ContextStructure::addChildParentRelation(mace::string const& child_ctx_name, ContextStructureNode* parent_node) {
  ScopedLock sl(contextStructureMutex);
  ContextStructureNode* child_node = this->findContextNodeNoLock(child_ctx_name);
  if(child_node == NULL) {
    return false;
  }

  parent_node->addChild(child_node);
  child_node->addParent(parent_node);
  return true;
}

mace::set<mace::string> ContextStructure::getAllDecendantContexts(mace::string const& ctx_name) const{
  ADD_SELECTORS("ContextStructure::getAllDecendantContexts");
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::set<mace::string> null_set;
    return null_set;
  }

  mace::set<mace::string> decendants;

  const ContextStructureNode::ContextStructureNodesType& child_contexts = node->child_contexts;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    mace::set<mace::string> ds = child_contexts[i]->getDecendantNames();

    for(mace::set<mace::string>::iterator iter = ds.begin(); iter != ds.end(); iter ++ ) {
      if( decendants.count(*iter) == 0 ) {
        decendants.insert( *iter );
      }
    }
    //macedbg(1) << decendants << Log::endl;
  }

  decendants.insert( node->getCtxName() );
  return decendants;
}

mace::set<mace::string> ContextStructure::getOrderingDecendantContexts(mace::string const& ctx_name) const{
  ADD_SELECTORS("ContextStructure::getOrderingDecendantContexts");
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::set<mace::string> null_set;
    return null_set;
  }

  mace::set<mace::string> decendants;

  const ContextStructureNode::ContextStructureNodesType& child_contexts = node->child_contexts;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    mace::set<mace::string> ds;
    if( !child_contexts[i]->isUpperBoundContext() ){
      ds = getOrderingDecendantContexts2(child_contexts[i]->getCtxName());
    }
    if( ds.count(child_contexts[i]->getCtxName()) == 0 ){
      ds.insert(child_contexts[i]->getCtxName());
    }

    for(mace::set<mace::string>::iterator iter = ds.begin(); iter != ds.end(); iter ++ ) {
      if( decendants.count(*iter) == 0 ) {
        decendants.insert( *iter );
      }
    }
    //macedbg(1) << decendants << Log::endl;
  }

  decendants.insert( node->getCtxName() );
  return decendants;
}

mace::set<mace::string> ContextStructure::getOrderingDecendantContexts2(mace::string const& ctx_name) const{
  ADD_SELECTORS("ContextStructure::getOrderingDecendantContexts2");
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::set<mace::string> null_set;
    return null_set;
  }

  mace::set<mace::string> decendants;

  const ContextStructureNode::ContextStructureNodesType& child_contexts = node->child_contexts;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    mace::set<mace::string> ds;
    if( !child_contexts[i]->isUpperBoundContext() ){
      ds = getOrderingDecendantContexts2(child_contexts[i]->getCtxName());
    }
    if( ds.count(child_contexts[i]->getCtxName()) == 0 ){
      ds.insert(child_contexts[i]->getCtxName());
    }

    for(mace::set<mace::string>::iterator iter = ds.begin(); iter != ds.end(); iter ++ ) {
      if( decendants.count(*iter) == 0 ) {
        decendants.insert( *iter );
      }
    }
    //macedbg(1) << decendants << Log::endl;
  }

  decendants.insert( node->getCtxName() );
  return decendants;
}

mace::vector<mace::string> ContextStructure::getAllChildContexts(mace::string const& ctx_name) const {
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::vector<mace::string> null_vector;
    return null_vector;
  }

  return node->getChildrenNames();
}

mace::vector<mace::string> ContextStructure::getAllChildContextsNoLock(mace::string const& ctx_name) const {
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::vector<mace::string> null_vector;
    return null_vector;
  }

  return node->getChildrenNames();
}

mace::vector<mace::string> ContextStructure::getAllParentContexts(mace::string const& ctx_name) const {
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    mace::vector<mace::string> null_vector;
    return null_vector;
  }

  return node->getParentNames();
}

mace::vector<mace::string> ContextStructure::getDominateContexts( mace::string const& ctxName ) const {
  ADD_SELECTORS("ContextStructure::getDominateContexts");
  mace::vector<mace::string> v_dominateContexts;
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctxName);

  if( node == NULL ){
    macedbg(1) << "Fail to find context("<< ctxName <<") in DAG!" << Log::endl;
    v_dominateContexts.push_back(ctxName);
    return v_dominateContexts;
  }

  if( !node->isUpperBoundContext() ) {
    // macedbg(1) << "context("<< ctxName <<") is not an UB!" << Log::endl;
    return v_dominateContexts;
  }
  mace::set<mace::string> s_dominateContexts;
  s_dominateContexts.insert(ctxName);

  std::vector< ContextStructureNode* > cnodes = node->getChildren();
  for( uint32_t i=0; i<cnodes.size(); i++ ) {
    cnodes[i]->getDominateContexts(s_dominateContexts);
  }

  for( mace::set<mace::string>::iterator sIter=s_dominateContexts.begin(); sIter!=s_dominateContexts.end(); sIter++ ){
    const mace::string& sDominateContext = *sIter;
    bool insert_flag = false;
    for( mace::vector<mace::string>::iterator vIter=v_dominateContexts.begin(); vIter!=v_dominateContexts.end(); vIter++ ){
      const mace::string& vDominateContext = *vIter;
      if( this->isElderContextNoLock(vDominateContext, sDominateContext) ){
        insert_flag = true;
        v_dominateContexts.insert( vIter, sDominateContext );
        break;
      }
    }

    if( !insert_flag ){
      v_dominateContexts.push_back(sDominateContext);
    }
  }

  return v_dominateContexts;
}


bool ContextStructure::isParentContext(mace::string const& child_ctx_name, mace::string const& parent_ctx_name) const{
  const ContextStructureNode* parent_node = this->const_findContextNodeNoLock(parent_ctx_name);

  if( parent_node == NULL ) {
    return false;
  } 

  mace::vector<mace::string> children = parent_node->getChildrenNames();
  for(uint32_t i=0; i<children.size(); i++) {
    if(children[i] == child_ctx_name) {
      return true;
    }
  }

  return false;
}

bool ContextStructure::isElderContext(mace::string const& d_ctx_name, mace::string const& elder_ctx_name) const {
  ScopedLock sl(contextStructureMutex);

  const ContextStructureNode* elder_node = this->const_findContextNodeNoLock(elder_ctx_name);

  if( elder_node == NULL ) {
    return false;
  } 

  if(d_ctx_name == elder_ctx_name) {
    return true;
  }

  mace::vector<mace::string> children = elder_node->getChildrenNames();
  for(uint32_t i=0; i<children.size(); i++) {
    if( this->isElderContextNoLock(d_ctx_name, children[i]) ) {
      return true;
    }    
  }
  return false;
}

bool ContextStructure::isElderContextNoLock(mace::string const& d_ctx_name, mace::string const& elder_ctx_name) const {
  const ContextStructureNode* elder_node = this->const_findContextNodeNoLock(elder_ctx_name);

  if( elder_node == NULL ) {
    return false;
  } 

  if(d_ctx_name == elder_ctx_name) {
    return true;
  }

  mace::vector<mace::string> children = elder_node->getChildrenNames();
  for(uint32_t i=0; i<children.size(); i++) {
    if( this->isElderContextNoLock(d_ctx_name, children[i]) ) {
      return true;
    }    
  }
  return false;
}

bool ContextStructure::isUpperBoundContext(mace::string const& ctx_name) const{
  ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    return false;
  } else {
    return node->isUpperBoundContext();
  }
}

bool ContextStructure::isUpperBoundContextNoLock(mace::string const& ctx_name) const{
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    return false;
  } else {
    return node->isUpperBoundContext();
  }
}

mace::string ContextStructure::getUpperBoundContextName(mace::string const& ctx_name) const {
  ADD_SELECTORS("ContextStructure::getUpperBoundContextName");
  //ScopedLock sl(contextStructureMutex);
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);

  if( node == NULL ) {
    // std::map<mace::string, ContextStructureNode*>::const_iterator citer = nameNodeMap.begin();
    macedbg(1) << "Fail to find context(" << ctx_name <<") in ownership DAG!" << Log::endl;
    mace::string ctxName = ctx_name;
    return ctxName;
  } else {
    mace::string ctxName = node->getUpperBoundContextName();
    return ctxName;
  }
}

mace::string ContextStructure::getDominatorContext(mace::string const& ctxName) {
  ADD_SELECTORS("ContextStructure::getDominatorContext");
  ScopedLock sl(contextStructureMutex);

  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctxName);

  if( node == NULL ) {
    // std::map<mace::string, ContextStructureNode*>::const_iterator citer = nameNodeMap.begin();
    maceerr << "Fail to find context(" << ctxName <<")'s dominator!" << Log::endl;
    return "NULL";
  } else {
    return node->getUpperBoundContextName();
  }
}

mace::string ContextStructure::getParentDominatorContext(mace::string const& ctxName) {
  ADD_SELECTORS("ContextStructure::getParentDominatorContext");
  ScopedLock sl(contextStructureMutex);

  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctxName);

  if( node == NULL ) {
    // std::map<mace::string, ContextStructureNode*>::const_iterator citer = nameNodeMap.begin();
    maceerr << "Fail to find context(" << ctxName <<")'s parent LUB!" << Log::endl;
    return "NULL";
  } else {
    const mace::vector<mace::string> parentContextNames = node->getParentNames();
    if( parentContextNames.size() == 0 ){
      return node->getUpperBoundContextName();
    } else {
      sl.unlock();
      return this->getDominatorContext( parentContextNames[0] );
    }
  }
}

void ContextStructure::traverseAndUpdateUpperBoundContextsInForest() {
  ADD_SELECTORS("ContextStructure::traverseAndUpdateUpperBoundContextsInForest");

  mace::set<mace::string> root_contexts;
  for( std::map<mace::string, ContextStructureNode*>::iterator iter = nameNodeMap.begin(); iter != nameNodeMap.end(); iter ++ ) {
    mace::string root_context = (iter->second)->getRootContextName();
    if( root_contexts.count(root_context) == 0 ) {
      root_contexts.insert(root_context);
    }
  }
  macedbg(1) << "Root contexts: " << root_contexts << Log::endl;
  for( mace::set<mace::string>::iterator iter = root_contexts.begin(); iter != root_contexts.end(); iter ++ ) {
    this->traverseAndUpdateUpperBoundContexts(*iter);
  }
}

void ContextStructure::traverseAndUpdateUpperBoundContexts( const mace::string& ctx_name ) {
  ADD_SELECTORS("ContextStructure::traverseAndUpdateUpperBoundContexts#1");
  // macedbg(1) << "Start to find upper bound contexts from " << ctx_name << Log::endl;

  mace::set<mace::string> children;
  mace::set<mace::string> parents;

  ContextStructureNode* node = this->findContextNodeNoLock(ctx_name);
  traverseAndUpdateUpperBoundContexts(node, children, parents);
}

void ContextStructure::traverseAndUpdateUpperBoundContexts(ContextStructureNode* node, mace::set<mace::string>& pNode_children, 
    mace::set<mace::string>& pNode_parents) {
  ADD_SELECTORS("ContextStructure::traverseAndUpdateUpperBoundContexts#2");
  if( node == NULL ) {
    return;
  }

  // macedbg(1) << "Start from context: " << node->getCtxName() << Log::endl;
  mace::set<mace::string> my_children;
  mace::set<mace::string> my_parents;

  const mace::string ctxName = node->getCtxName();

  my_children.insert(ctxName);
  my_parents.insert(ctxName);
  
  if( pNode_children.count(ctxName) == 0 ) pNode_children.insert(ctxName);
  if( pNode_parents.count(ctxName) == 0 ) pNode_parents.insert(ctxName);

  mace::vector<mace::string> parentNames = node->getParentNames();
  for( uint32_t i=0; i < parentNames.size(); i++ ) {
    if( pNode_parents.count(parentNames[i]) == 0 ){
      pNode_parents.insert(parentNames[i]);
    }
  }

  ContextStructureNode::ContextStructureNodesType children = node->getChildren();
  if( children.size() == 0 ) { // leaf context
    node->setIsUpperBoundContext(true);
    node->setUpperBoundContextName( ctxName );
    // macedbg(1) << "#1 UBC relationship: " << node->getCtxName() << "->" << node->getCtxName() << Log::endl;
    return;
  }


  for(uint32_t i = 0; i < children.size(); i++) {
    ContextStructureNode* cnode = children[i];
    // macedbg(1) << "To check child("<< cnode->getCtxName() <<") in parent("<< ctxName <<")!" << Log::endl;
    traverseAndUpdateUpperBoundContexts(cnode, my_children, my_parents);
  }

  bool root_flag = true;
  mace::set<mace::string>::iterator iter = my_parents.begin();
  for( ; iter != my_parents.end(); iter++ ) {
    if( my_children.count(*iter) == 0 ) {
      root_flag = false;
      break;
    } 
  }

  
  if( root_flag ) {
    for(uint32_t i = 0; i < children.size(); i++) {
      ContextStructureNode* cnode = children[i];
      broadcastUpperBoundContext(cnode, ctxName);
    }
    // macedbg(1) << "#2 UBC relationship: " << node->getCtxName() << "->" << node->getCtxName() << Log::endl;
    node->setUpperBoundContextName(ctxName);
  } else {
    for( iter = my_parents.begin(); iter != my_parents.end(); iter++ ) {
      if( pNode_parents.count(*iter) == 0 ) pNode_parents.insert(*iter);
    }

    for( iter = my_children.begin(); iter != my_children.end(); iter++ ) {
      if( pNode_children.count(*iter) == 0 ) pNode_children.insert(*iter);
    }

  } 

  node->setIsUpperBoundContext(root_flag);
}

void ContextStructure::broadcastUpperBoundContext(ContextStructureNode* node, mace::string const& ctx_name) {
  ADD_SELECTORS("ContextStructure::broadcastUpperBoundContext");
  if(node == NULL || node->isUpperBoundContext()) {
    return;
  }
  
  node->setUpperBoundContextName(ctx_name);
  
  macedbg(1) << "UBC relationship: " << ctx_name << "->" << node->getCtxName() << Log::endl;
  ContextStructureNode::ContextStructureNodesType children = node->getChildren();
  for(uint32_t i = 0; i < children.size(); i++) {
    ContextStructureNode* cnode = children[i];
    broadcastUpperBoundContext(cnode, ctx_name);
  }
}

ContextStructureNode* ContextStructure::findContextNodeNoLock(mace::string const& ctx_name) {
  std::map<mace::string, ContextStructureNode*>::iterator iter = nameNodeMap.find(ctx_name);
  if( iter == nameNodeMap.end() ) {
    return NULL;
  } else {
    return iter->second;
  }
}

const ContextStructureNode* ContextStructure::const_findContextNodeNoLock(mace::string const& ctx_name) const {
  std::map<mace::string, ContextStructureNode*>::const_iterator c_iter = nameNodeMap.find(ctx_name);
  if( c_iter == nameNodeMap.end() ) {
    return NULL;
  } else {
    return c_iter->second;
  }
}

mace::vector<mace::string> ContextStructureNode::getParentNames() const {
  mace::vector<mace::string> parentNames;

  for(uint32_t i=0; i<parent_contexts.size(); i++) {
    ContextStructureNode* pnode = parent_contexts[i];
    parentNames.push_back( pnode->getCtxName() );
  }

  return parentNames;
}

mace::vector<mace::string> ContextStructureNode::getChildrenNames() const {
  mace::vector<mace::string> childrenNames;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    ContextStructureNode* cnode = child_contexts[i];
    childrenNames.push_back( cnode->getCtxName() );
  }

  return childrenNames;
}

mace::set<mace::string> ContextStructureNode::getDecendantNames() {
  mace::set<mace::string> dNames;

  dNames.insert(context_name);

  if( !isUpperBoundContextFlag ) {
    for(uint32_t i=0; i<child_contexts.size(); i++) {
      ContextStructureNode* cnode = child_contexts[i];
      mace::set<mace::string> cdnames = cnode->getDecendantNames();

      for( mace::set<mace::string>::iterator iter = cdnames.begin(); iter != cdnames.end(); iter++ ) {
        if( dNames.count(*iter) == 0 ) {
          dNames.insert(*iter);
        }
      }
    }
  }

  return dNames;
}

void ContextStructureNode::getDominateContexts( mace::set<mace::string>& dominateContexts ) const {
  ADD_SELECTORS("ContextStructureNode::getDominateContexts");
  if( dominateContexts.count(context_name) == 0 ) {
    dominateContexts.insert( context_name );
    // macedbg(1) << "Adding dominated context " << context_name << Log::endl;
  }

  if( isUpperBoundContextFlag ){
    // macedbg(1) << "context("<< context_name <<") is an UB. Return!" << Log::endl;
    return;
  }

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    ContextStructureNode* cnode = child_contexts[i];
    cnode->getDominateContexts( dominateContexts );
  }
}

void ContextStructureNode::getAllContextDominators( mace::map<mace::string, mace::string>& dominators ) const {
  dominators[this->context_name] = this->upper_bound_ctx_name;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    ContextStructureNode* cnode = child_contexts[i];
    cnode->getAllContextDominators( dominators );
  }
}

void ContextStructureNode::getAllParentContextDominators( mace::string const& pDominator, mace::map<mace::string, mace::string>& dominators ) const {
  dominators[this->context_name] = pDominator;

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    ContextStructureNode* cnode = child_contexts[i];
    cnode->getAllParentContextDominators( this->upper_bound_ctx_name, dominators );
  }
}

void ContextStructureNode::getDominateRegionContexts( const mace::string& dominator,
    mace::map<mace::string, mace::set<mace::string> >& dominateContexts ) const {
  
  if( dominateContexts[dominator].count(this->context_name) == 0 ){
    dominateContexts[dominator].insert(this->context_name);
  }

  mace::string next_dominator = dominator;
  if( isUpperBoundContextFlag ) {
    next_dominator = this->context_name;
    if( dominateContexts[this->context_name].count(this->context_name) == 0 ) {
      dominateContexts[this->context_name].insert(this->context_name);
    }
  }

  for(uint32_t i=0; i<child_contexts.size(); i++) {
    ContextStructureNode* cnode = child_contexts[i];
    cnode->getDominateRegionContexts( next_dominator, dominateContexts);
  }
}

bool ContextStructureNode::connectToNode( const mace::string& ctx_name ) const {
  if( context_name == ctx_name ) {
    return true;
  }

  bool connected = false;
  for( ContextStructureNodesType::const_iterator iter = child_contexts.begin(); iter != child_contexts.end(); iter ++ ){
    if( (*iter)->connectToNode(ctx_name) ) {
      connected = true;
      break;
    }
  }
  return connected;
}

/********************************************* class ContextStructure *****************************************************/

void ContextStructure::constructContextStructure() {
  ADD_SELECTORS("ContextStructure::constructContextStructure");
  macedbg(1) << "Start to construct ContextStructure " << Log::endl;

  ScopedLock sl(contextStructureMutex);

  rootNode = new ContextStructureNode(mace::ContextMapping::GLOBAL_CONTEXT_NAME, "", mace::ContextMapping::HEAD_CONTEXT_ID, true);
  nameNodeMap[mace::ContextMapping::GLOBAL_CONTEXT_NAME] = rootNode;


  std::map<mace::string, ContextStructureNode*> tempNodeMap;
  mace::vector< mace::pair<mace::string, mace::string> > ownerships = ContextStructure::getInitialContextOwnerships();
  
  for(uint32_t i = 0;  i<ownerships.size(); i++) {
    mace::pair<mace::string, mace::string>& ownership = ownerships[i];
    contextOwnerships.insert(ownership);

    mace::string parent_ctx_name = ownership.first;
    mace::string child_ctx_name = ownership.second;



    ContextStructureNode* pNode = findContextNodeNoLock(parent_ctx_name);
    ContextStructureNode* cNode = findContextNodeNoLock(child_ctx_name);
    if( pNode != NULL && cNode != NULL ) {
      ContextStructureNode::setParentChildRelation(pNode, cNode);
      macedbg(1) << "Set ownership #1: " << pNode->getCtxName() << "->" << cNode->getCtxName() << Log::endl;
    } else if (pNode != NULL ) {
      std::map<mace::string, ContextStructureNode*>::iterator tempIter = tempNodeMap.find(child_ctx_name);
      if(tempIter != tempNodeMap.end() ) {
        cNode = tempIter->second;
        tempNodeMap.erase(tempIter);
      } else {
        cNode = new ContextStructureNode(child_ctx_name, "", 0);
      }
      this->addParentChildRelationNoLock(parent_ctx_name, cNode);
      nameNodeMap[child_ctx_name] = cNode;
      macedbg(1) << "Set ownership #2: " << parent_ctx_name << "->" << cNode->getCtxName() << Log::endl;
    } else if (cNode != NULL) {
      std::map<mace::string, ContextStructureNode*>::iterator tempIter = tempNodeMap.find(parent_ctx_name);
      if(tempIter != tempNodeMap.end() ) {
        pNode = tempIter->second;
        tempNodeMap.erase(tempIter);
      } else {
        pNode = new ContextStructureNode(parent_ctx_name, "", 0);
      }
      this->addChildParentRelation(child_ctx_name, pNode);
      nameNodeMap[parent_ctx_name] = pNode;
      macedbg(1) << "Set ownership #3: " << pNode->getCtxName() << "->" << child_ctx_name << Log::endl;
    } else {
      std::map<mace::string, ContextStructureNode*>::iterator tempIter = tempNodeMap.find(child_ctx_name);
      if(tempIter != tempNodeMap.end() ) {
        cNode = tempIter->second;
      } else {
        cNode = new ContextStructureNode(child_ctx_name, "", 0);
        tempNodeMap[child_ctx_name] = cNode;
      }

      tempIter = tempNodeMap.find(parent_ctx_name);
      if(tempIter != tempNodeMap.end() ) {
        pNode = tempIter->second;
      } else {
        pNode = new ContextStructureNode(parent_ctx_name, "", 0);
        tempNodeMap[parent_ctx_name] = pNode;
      }

      ContextStructureNode::setParentChildRelation(pNode, cNode);
      macedbg(1) << "Set ownership #4: " << pNode->getCtxName() << "->" << cNode->getCtxName() << Log::endl;
    }

  }
  mace::vector<mace::string> children = this->getAllChildContextsNoLock(mace::ContextMapping::GLOBAL_CONTEXT_NAME);
  macedbg(1) << "Global context's children: " << children << Log::endl;

  traverseAndUpdateUpperBoundContexts(mace::ContextMapping::GLOBAL_CONTEXT_NAME);  
}

void ContextStructure::printAllOwnerships() const {
  ADD_SELECTORS("ContextStructure::printAllOwnerships");
  for(mace::set< mace::pair<mace::string, mace::string> >::const_iterator cIter = contextOwnerships.begin(); 
      cIter != contextOwnerships.end(); cIter++ ) {
    macedbg(1) << *cIter << Log::endl;
  }
}

void ContextStructure::clearUnusedContextStructureNodes() {
  ADD_SELECTORS("ContextStructure::clearUnusedContextStructureNodes");
  mace::set<mace::string> contexts;
  for( mace::set< mace::pair<mace::string, mace::string> >::const_iterator iter = contextOwnerships.begin(); 
      iter != contextOwnerships.end(); iter ++ ) {
    const mace::pair<mace::string, mace::string>& edge = *iter;
    if( contexts.count(edge.first) == 0 ){
      contexts.insert(edge.first);
    }

    if( contexts.count(edge.second) == 0 ) {
      contexts.insert(edge.second);
    }
  }

  mace::set<mace::string> toDelete;
  for( std::map<mace::string, ContextStructureNode*>::iterator iter = nameNodeMap.begin(); iter != nameNodeMap.end(); 
      iter ++ ){
    if( contexts.count(iter->first) == 0 ){
      toDelete.insert(iter->first);
      delete iter->second;
      iter->second = NULL;
    }
  }

  for( mace::set<mace::string>::const_iterator iter = toDelete.begin(); iter != toDelete.end(); iter ++) {
    macedbg(1) << "Remove context("<< *iter <<") from DAG!" << Log::endl;
    nameNodeMap.erase(*iter);
  }

}

bool ContextStructure::checkParentChildRelation(mace::string const& p, mace::string const& c) const {
  mace::pair<mace::string, mace::string> ownership(p, c);
  if( contextOwnerships.count(ownership) ) {
    return true;
  } else {
    return false;
  }
}


mace::set<mace::string> ContextStructure::modifyOwnerships( const mace::string& dominator, 
    const mace::vector< mace::EventOperationInfo >& ownershipOpInfos) {
  ADD_SELECTORS("ContextStructure::modifyOwnerships");
  ContextStructureNode* dom_node = this->findContextNodeNoLock(dominator);
  ASSERT( dom_node != NULL );

  mace::map< mace::string, mace::set<mace::string> > oldDominateContexts;
  dom_node->getDominateRegionContexts( dom_node->getCtxName(), oldDominateContexts);
  mace::set< mace::string> affected_contexts;

  bool modified_flag = false;
  ScopedLock sl(contextStructureMutex);
  for(uint64_t i=0; i<ownershipOpInfos.size(); i++ ){
    const mace::EventOperationInfo& opInfo = ownershipOpInfos[i];
    // macedbg(1) << "Try to apply ownership operation: " << opInfo << Log::endl;
    if( opInfo.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && 
        !checkParentChildRelation(opInfo.fromContextName, opInfo.toContextName) ) {
      modified_flag = true;
      macedbg(1) << "To add new ownership from "<< opInfo.fromContextName << " to " << opInfo.toContextName << Log::endl;
      ContextStructureNode* pNode = findContextNodeNoLock(opInfo.fromContextName);
      if( pNode == NULL ){
        pNode = new ContextStructureNode(opInfo.fromContextName, "", 0);
        nameNodeMap[opInfo.fromContextName] = pNode;
      }
      ContextStructureNode* cNode = findContextNodeNoLock(opInfo.toContextName);
      if( cNode == NULL ){
        cNode = new ContextStructureNode(opInfo.toContextName, "", 0);
        nameNodeMap[opInfo.toContextName] = cNode;
      } else {
        if( isElderContextNoLock(opInfo.fromContextName, opInfo.toContextName) ) {
          maceerr << "ERROR: "<< opInfo.fromContextName << " is the child of " << opInfo.toContextName << ". This ownership operation is invalid!" << Log::endl;
          continue;
        }
      }
      ContextStructureNode::setParentChildRelation(pNode, cNode);
      pNode->increaseVersion();
      mace::pair<mace::string, mace::string> nOwnership( opInfo.fromContextName, opInfo.toContextName );
      contextOwnerships.insert(nOwnership);
      if( affected_contexts.count(opInfo.toContextName) == 0 ) {
        affected_contexts.insert(opInfo.toContextName);
      }

      if( affected_contexts.count(opInfo.fromContextName) == 0 ) {
        affected_contexts.insert(opInfo.fromContextName);
      }
    } else if ( opInfo.opType == mace::EventOperationInfo::DELETE_OWNERSHIP_OP && 
        checkParentChildRelation(opInfo.fromContextName, opInfo.toContextName) ) {
      modified_flag = true;
      macedbg(1) << "To delete ownership from "<< opInfo.fromContextName << " to " << opInfo.toContextName << Log::endl;
      ContextStructureNode* pNode = findContextNodeNoLock(opInfo.fromContextName);
      ContextStructureNode* cNode = findContextNodeNoLock(opInfo.toContextName);
      mace::pair<mace::string, mace::string> toDeleteOwnership(opInfo.fromContextName, opInfo.toContextName);
      contextOwnerships.erase(toDeleteOwnership);
      ASSERT( pNode != NULL && cNode != NULL );
      ContextStructureNode::deleteParentChildRelation( pNode, cNode );
      pNode->increaseVersion();
      if( affected_contexts.count(opInfo.toContextName) == 0 ) {
        affected_contexts.insert(opInfo.toContextName);
      }

      if( affected_contexts.count(opInfo.fromContextName) == 0 ) {
        affected_contexts.insert(opInfo.fromContextName);
      }
    }
  }

  if( !modified_flag ){
    return affected_contexts;
  }
  
  // remove unused node
  // clearUnusedContextStructureNodes();

  dom_node = this->findContextNodeNoLock(dominator);
  ASSERT( dom_node != NULL );
  // clear upper bound flag
  clearUpperBoundContextFlag(dom_node);
  // decide new upper bound context
  traverseAndUpdateUpperBoundContexts(dominator);

  sl.unlock();

  ASSERT( dom_node->isUpperBoundContext() );

  mace::map< mace::string, mace::set<mace::string> > newDominateContexts;
  dom_node->getDominateRegionContexts( dom_node->getCtxName(), newDominateContexts );

  macedbg(1) << "newDominateContexts: " << newDominateContexts << Log::endl;
  macedbg(1) << "oldDominateContexts: " << oldDominateContexts << Log::endl;
  
  for( mace::map< mace::string, mace::set<mace::string> >::iterator iter=newDominateContexts.begin(); iter!=newDominateContexts.end(); iter++ ){
    if( oldDominateContexts.find(iter->first) == oldDominateContexts.end() && affected_contexts.count(iter->first) == 0 ){
      affected_contexts.insert(iter->first);
    } else {
      const mace::set<mace::string>& newSet = iter->second;
      const mace::set<mace::string>& oldSet = oldDominateContexts[ iter->first ];
      if( oldSet.size() != newSet.size() && affected_contexts.count(iter->first) == 0 ){
        affected_contexts.insert( iter->first );
      } else {
        for( mace::set<mace::string>::const_iterator sIter=oldSet.begin(); sIter!=oldSet.end(); sIter++ ){
          if( newSet.count(*sIter) == 0 && affected_contexts.count(iter->first) == 0 ){
            affected_contexts.insert( iter->first );
            break;
          }
        }
      }
    }
  }

  for( mace::map< mace::string, mace::set<mace::string> >::iterator iter=oldDominateContexts.begin(); iter!=oldDominateContexts.end(); iter++ ){
    if( newDominateContexts.find(iter->first) == newDominateContexts.end() && affected_contexts.count(iter->first) == 0 ){
      affected_contexts.insert(iter->first);
    } 
  }

  return affected_contexts;
}

mace::string ContextStructure::getParentContextName( mace::string const& ctxName ) const {
  ScopedLock sl(contextStructureMutex);
  mace::string parentContextName = "";
  std::map<mace::string, ContextStructureNode*>::const_iterator c_iter = nameNodeMap.find(ctxName);
  if( c_iter != nameNodeMap.end() ) {
    const ContextStructureNode* node = c_iter->second;
    mace::vector<mace::string> parentContextNames = node->getParentNames();
    if( parentContextNames.size() > 0 ) {
      parentContextName = parentContextNames[0];
    }
  }
  return parentContextName;
}

void ContextStructure::updateOwnerships( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, 
    const mace::map<mace::string, uint64_t>& vers ) {
  ADD_SELECTORS("ContextStructure::updateOwnership");
  ScopedLock sl(contextStructureMutex);
  
  macedbg(1) << "Receving ownerships: " << ownerships << ", vers: " << vers << Log::endl;

  mace::set< mace::string > to_update_contexts;
  mace::set< mace::pair<mace::string, mace::string> > update_opairs;
  mace::map<mace::string, uint64_t>::const_iterator vIter;
  for( uint64_t i=0; i<ownerships.size(); i++ ){
    const mace::pair<mace::string, mace::string>& opair = ownerships[i];
    ContextStructureNode* node = this->findContextNodeNoLock( opair.first );
    vIter = vers.find(opair.first);
    if( node == NULL || vIter->second > node->getVersion() ) {
      if( to_update_contexts.count(opair.first) == 0 ) {
        to_update_contexts.insert(opair.first);
      }
      update_opairs.insert(opair);
    }
  }

  macedbg(1) << "to_update_contexts: " << to_update_contexts << Log::endl;
  
  if( to_update_contexts.size() == 0 ){
    return;
  }

  // macedbg(1) << "Try to update ownership for "<< vers << Log::endl;
  
  // add new ownerships
  for(uint64_t i=0; i<ownerships.size(); i++ ){
    const mace::pair<mace::string, mace::string>& opair = ownerships[i];
    if( to_update_contexts.count(opair.first) == 0 ){
      continue;
    }
    if( !checkParentChildRelation(opair.first, opair.second) ) {
      ContextStructureNode* pNode = findContextNodeNoLock(opair.first);
      if( pNode == NULL ) {
        // macedbg(1) << "Create new node for " << opair.first << Log::endl;
        pNode = new ContextStructureNode(opair.first, "", 0);
        nameNodeMap[ opair.first ] = pNode;
      }
      ContextStructureNode* cNode = findContextNodeNoLock(opair.second);
      if( cNode == NULL ){
        cNode = new ContextStructureNode(opair.second, "", 0);
        nameNodeMap[ opair.second ] = cNode;
      } else {
        if( isElderContextNoLock(opair.first, opair.second) ) {
          maceerr << "ERROR: "<< opair.second << " is the child of " << opair.first << ". This ownership operation is invalid!" << Log::endl;
          continue;
        }
      }
      ContextStructureNode::setParentChildRelation(pNode, cNode);
      vIter = vers.find( pNode->getCtxName() );
      if( pNode->getVersion() < vIter->second ) {
        pNode->setVersion( vIter->second );
      }
      vIter = vers.find(cNode->getCtxName());
      if( cNode->getVersion() < vIter->second ) {
        cNode->setVersion( vIter->second );
      }
      mace::pair<mace::string, mace::string> nOwnership( opair.first, opair.second );
      contextOwnerships.insert(nOwnership);

      // mace::vector<mace::string> children_contexts = pNode->getChildrenNames();
      // macedbg(1) << "context("<< pNode->getCtxName() <<")'s children: " << children_contexts << Log::endl;
    }
  }

  // remove deleted ownerships
  bool to_continue = true;
  while(to_continue) {
    to_continue = false;
    for( mace::set< mace::pair<mace::string, mace::string> >::iterator iter = contextOwnerships.begin(); iter != contextOwnerships.end(); 
        iter ++ ) {
      const mace::pair<mace::string, mace::string>& opair = *iter;
      if( to_update_contexts.count(opair.first) > 0 && update_opairs.count(opair) == 0  ) {
        ContextStructureNode* pNode = findContextNodeNoLock(opair.first);
        ContextStructureNode* cNode = findContextNodeNoLock(opair.second);
        ASSERT(pNode != NULL && cNode != NULL );
        ContextStructureNode::deleteParentChildRelation(pNode, cNode);
        contextOwnerships.erase(iter);
        to_continue = true;
        break;
      }
    }
  }
    
  // clearUnusedContextStructureNodes();
  //clearUpperBoundContextFlag(rootNode);
  traverseAndUpdateUpperBoundContextsInForest();
}

mace::vector< mace::pair<mace::string, mace::string> > ContextStructure::getAllOwnerships() const {
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  ScopedLock sl(contextStructureMutex);
  getAllOwnerships(rootNode, ownerships);
  return ownerships;
}

mace::vector< mace::pair<mace::string, mace::string> > ContextStructure::getOwnershipOfContexts( 
    const mace::set<mace::string>& contexts ) const {
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  for( mace::set<mace::string>::const_iterator iter = contexts.begin(); iter != contexts.end(); iter ++ ) {
    const ContextStructureNode* node = this->const_findContextNodeNoLock( *iter );
    if( node == NULL ){
      continue;
    }
    mace::vector<mace::string> childrenNames = node->getChildrenNames();
    for( uint32_t i=0; i<childrenNames.size(); i++ ) {
      mace::pair<mace::string, mace::string> ownership( node->getCtxName(), childrenNames[i] );
      ownerships.push_back(ownership);
    }
  }
  return ownerships;
}

uint64_t ContextStructure::getDAGNodeVersion( const mace::string& ctx_name) const {
  const ContextStructureNode* node = this->const_findContextNodeNoLock(ctx_name);
  if( node == NULL ){
    return 0;
  } else {
    return node->getVersion();
  }
}

void ContextStructure::getAllOwnerships( const ContextStructureNode* node, 
    mace::vector< mace::pair<mace::string, mace::string> >& ownerships) const {
  if( node == NULL ) {
    return;
  }

  const ContextStructureNode::ContextStructureNodesType& children = node->getChildren();
  for( ContextStructureNode::ContextStructureNodesType::const_iterator c_iter = children.begin(); c_iter != children.end(); c_iter ++ ) {
    mace::pair<mace::string, mace::string> ownership( node->getCtxName(), (*c_iter)->getCtxName() );
    ownerships.push_back(ownership);
    getAllOwnerships( (*c_iter), ownerships );
  }
}

bool ContextStructure::connectToRootNode( const mace::string& ctx_name ) const {
  if( rootNode == NULL ){
    return false;
  }
  return rootNode->connectToNode(ctx_name);
}

mace::set<mace::string> ContextStructure::getContextsClosedToRoot( const mace::set<mace::string>& contexts ) const {
  mace::set<mace::string> closest_contexts;
  
  mace::vector<mace::string> sorted_contexts = this->sortContexts(contexts);
  if( sorted_contexts.size() == 0 ) {
    return closest_contexts;
  }

  closest_contexts.insert( sorted_contexts[0] );
  for( uint32_t i=1; i<sorted_contexts.size(); i++ ) {
    bool closest = true;
    for( mace::set<mace::string>::iterator iter = closest_contexts.begin(); iter != closest_contexts.end(); iter ++ ){
      if( this->isElderContext(sorted_contexts[i], *iter) ) {
        closest = false;
        break;
      }
    }
    if( closest ){
      closest_contexts.insert( sorted_contexts[i] );
    }
  }
  return closest_contexts;
}

mace::vector<mace::string> ContextStructure::sortContexts( const mace::set<mace::string>& contexts ) const {
  mace::vector<mace::string> sorted_contexts;
  if( rootNode == NULL ){
    return sorted_contexts;
  }

  for( mace::set<mace::string>::const_iterator iter1 = contexts.begin(); iter1 != contexts.end(); iter1 ++ ) {
    bool inserted = false;
    for( mace::vector<mace::string>::iterator iter2 = sorted_contexts.begin(); iter2 != sorted_contexts.end(); iter2 ++ ){
      if( this->isElderContext(*iter2, *iter1) ) {
        inserted = true;
        sorted_contexts.insert(iter2, *iter1);
        break;
      }
    }
    if( !inserted ) {
      sorted_contexts.push_back(*iter1);
    }
  }
  return sorted_contexts;
}

void ContextStructure::clearUpperBoundContextFlag( ContextStructureNode* root ) {
  if(root == NULL) {
    return;
  }
  
  ContextStructureNode::ContextStructureNodesType children = root->getChildren();
  for(uint64_t i = 0; i < children.size(); i ++) {
    ContextStructureNode* node = children[i];
    clearUpperBoundContextFlag(node);
  }
  root->setIsUpperBoundContext(false);
  
}

void ContextStructure::getLock( const uint8_t lock_type ) {
  ADD_SELECTORS("ContextStructure::getLock");
  
  // macedbg(1) << "Try to get lock=" << (uint16_t)lock_type << " waitingConds.size="<< waitingConds.size() << " nreader="<< nreader << " nwriter=" << nwriter << Log::endl;
  ScopedLock sl( contextStructureMutex );
  if( lock_type == ContextStructure::READER ) {
    if( waitingConds.size() == 0 && nwriter == 0 ){
      nreader ++;
      // macedbg(1) << "Get lock=" << (uint16_t)lock_type << Log::end;
    } else {
      const std::pair<uint8_t, pthread_cond_t*>& next = waitingConds.front();
      
      macedbg(1) << "nreader=" << nreader << " nwriter=" << nwriter << " nextWaiting=" << (uint16_t)next.first << Log::endl;

      pthread_cond_t cond;
      pthread_cond_init(&cond, NULL);

      std::pair<uint8_t, pthread_cond_t*> p(lock_type, &cond);
      waitingConds.push_back(p);
      // macedbg(1) << "Waiting... waitingConds.size="<< waitingConds.size() << " nreader="<< nreader << " nwriter=" << nwriter << Log::endl;
      pthread_cond_wait( &cond, &contextStructureMutex );
      pthread_cond_destroy(&cond);
      // macedbg(1) << "Wakeup!!! nreader=" << nreader << " nwriter=" << nwriter << Log::end;
      ASSERT( nwriter == 0 );
      //nreader ++;
    }
  } else if( lock_type == ContextStructure::WRITER ) {
    if( waitingConds.size() == 0 && nwriter == 0 && nreader == 0 ) {
      nwriter ++;
      // macedbg(1) << "Get lock=" << (uint16_t)lock_type << Log::end;
    } else {
      pthread_cond_t cond;
      pthread_cond_init(&cond, NULL);

      std::pair<uint8_t, pthread_cond_t*> p(lock_type, &cond);
      waitingConds.push_back(p);
      // macedbg(1) << "Waiting... waitingConds.size="<< waitingConds.size() << " nreader="<< nreader << " nwriter=" << nwriter << Log::endl;
      pthread_cond_wait( &cond, &contextStructureMutex );
      pthread_cond_destroy(&cond);
      // macedbg(1) << "Wakeup!!! nreader=" << nreader << " nwriter=" << nwriter << Log::end;
      ASSERT( nwriter == 1 && nreader == 0 );
    }
  }
  
}

void ContextStructure::releaseLock( const uint8_t lock_type ) {
  ADD_SELECTORS("ContextStructure::releaseLock");
  
  ScopedLock sl( contextStructureMutex );
  // macedbg(1) << "To release lock_type=" << (uint16_t)lock_type << " nreader=" << nreader << " nwriter="<< nwriter << Log::endl;
  if( lock_type == ContextStructure::READER ) {
    ASSERT( nreader > 0 );
    nreader --;
  } else if( lock_type == ContextStructure::WRITER ) {
    ASSERT( nwriter == 1 );
    nwriter --;
  } else {
    ASSERT(false);
  }

  if( nreader == 0 && nwriter == 0 ){
    bool first_locker = true;
    while( waitingConds.size() > 0 ) {
      std::pair<uint8_t, pthread_cond_t*>& p = waitingConds.front();

      if( p.first == ContextStructure::READER ) {
        // macedbg(1) << "Release lock=" << (uint16_t)p.first << Log::endl;
        pthread_cond_t* cond = p.second;
        p.second = NULL;
        pthread_cond_signal( cond ); nreader ++;
        waitingConds.pop_front();
      } else if( p.first == ContextStructure::WRITER ) {
        if( first_locker ) {
          // macedbg(1) << "Release lock=" << (uint16_t)p.first << Log::endl;
          pthread_cond_t* cond = p.second;
          p.second = NULL;
          pthread_cond_signal( cond ); nwriter ++;
          waitingConds.pop_front();
        }
        break;
      }
      first_locker = false;
    }
  }
  
}

mace::map<mace::string, mace::string> ContextStructure::getAllContextDominators() const {
  ScopedLock sl(contextStructureMutex);

  mace::map<mace::string, mace::string> dominators;
  if( rootNode!=NULL ){
    rootNode->getAllContextDominators( dominators );
  }  

  return dominators;
}

mace::map<mace::string, mace::string> ContextStructure::getAllParentContextDominators() const {
  ScopedLock sl(contextStructureMutex);

  mace::map<mace::string, mace::string> dominators;
  if( rootNode!=NULL ){
    rootNode->getAllParentContextDominators( rootNode->getCtxName(), dominators );
  }  

  return dominators;
}

mace::map<mace::string, mace::set<mace::string> > ContextStructure::getDominateRegionContexts() const {
  ScopedLock sl(contextStructureMutex);
  mace::map<mace::string, mace::set<mace::string> > dominateContexts;
  if( rootNode != NULL ){
    rootNode->getDominateRegionContexts( rootNode->getCtxName(), dominateContexts );
  } 

  return dominateContexts;
}