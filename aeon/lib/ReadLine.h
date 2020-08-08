#ifndef READLINE_H
#define READLINE_H

#include "ThreadStructure.h"
#include "ContextMapping.h"
#include "mset.h"
#include "mstring.h"
#include <utility>
#include "Event.h"
#include "mhash_map.h"

namespace mace
{
/**
 * ReadLine class implements the concept of access permission in FullContext model 
 *
 * */
class ReadLine{
private:
  class TreeNode{
    public:
      TreeNode *prev;
      TreeNode *next;
      TreeNode *childlist;
      uint32_t contextID;
    public:
      TreeNode( const uint32_t id ): prev(NULL), next(NULL),childlist(NULL), contextID( id ) { 
      
      }
      /*void addChild( TreeNode* child ){ 
        child->prev = NULL;
        child->next = childlist;
        childlist = child;
      }*/
  };
public:
    /**
     * constructor
     *
     * computes the cut set of the read line
     *
     * @param contextMapping the current context map object
     *
     * */
    ReadLine( const ContextMapping& contextMapping):
      contextMapping( contextMapping  ) {
      ADD_SELECTORS("ReadLine::(constructor)");
      const Event::EventServiceContextType & eventContexts = ThreadStructure::getCurrentServiceEventContexts( );

      //macedbg(1)<<"the current context set of the event is : "<< eventContexts<< Log::endl;

      // find the cut of the read line.
      // algorithm: 
      // (1) Initially, create a list of n tree nodes: that is, assuming all of them are in the cut set.
      TreeNode head( 0 ); // head points to the read-line cut set
      prev = &head;
      const Event::EventServiceSnapshotContextType & snapshotContextNames = ThreadStructure::getCurrentServiceEventSnapshotContexts( );
      //macedbg(1)<<"the current context snapshot set of the event is : "<< snapshotContextNames<< Log::endl;
      for( Event::EventServiceSnapshotContextType::const_iterator ctxIt = snapshotContextNames.begin(); ctxIt != snapshotContextNames.end(); ctxIt++ ){
        eventSnapshotContexts.push_back( ctxIt->first );
      }
      while( eventSnapshotContexts.size() > 0 ){
        set<uint32_t>::iterator ctxIt = eventSnapshotContexts.begin();
        const uint32_t contextID = *ctxIt;
        eventSnapshotContexts.erase( ctxIt );
        recursivelyAddReadyOnlyContext( contextID );
      }
      for( Event::EventServiceContextType::const_iterator ctxIt = eventContexts.begin(); ctxIt != eventContexts.end(); ctxIt++ ){
        const uint32_t contextID = *ctxIt; 
        if( ctxNodes.count(  contextID ) == 0 ){ // entry did not exist before
          appendCandidateContext( contextID);
        }
      }
      // (2) For each tree node, if its parent is in the set(trivial to find using set::find() ), create the link between parent and that node
      unionContexts(head );

      // (3) Finally, the remaining nodes are the root of the subtrees, and they are the read-line cut.
      TreeNode *node = head.next;
      node = head.next;
      while( node != NULL ){
        cutSet.push_back( node->contextID );
        node = node->next;
      }
      
      //
      // So it's an O( n* log n ) algorithm (assuming map insertion is O(log n) )
    }
    /**
     * returns the computed read line cut set
     *
     * @return read line cut set
     * */
    const list< uint32_t >& getCut(){
      return cutSet;
    }

    /**
     * destructor
     *
     * delete tree structure nodes
     *
     * */
    ~ReadLine(){

      // (4) cleanup
      for( map< uint32_t, TreeNode*, SoftState >::iterator ctxIt = ctxNodes.begin(); ctxIt != ctxNodes.end(); ctxIt ++ ){
        delete ctxIt->second;
      }
    }
private:
  // for a context that we have snapshot, it's already committed, so don't need to commit it.
  // but the event still need to enter its child contexts to commit them.
    void recursivelyAddReadyOnlyContext(const uint32_t contextID ){
      ADD_SELECTORS("ReadLine::(constructor)");
      /*
        std::queue< uint32_t > contextIDs;
        contextIDs.push( contextID );

        while( !contextIDs.empty()){
          const uint32_t targetContextID = contextIDs.front();
          contextIDs.pop();
          const set< uint32_t >& childnodes = ContextMapping::getChildContexts(contextMapping, targetContextID );

          //macedbg(1)<<"child context set of context "<< targetContextID <<": "<< childnodes <<Log::endl;

          if( childnodes.size() == 0 ){ continue; }

          
          for( set< uint32_t >::const_iterator cnIt = childnodes.begin(); cnIt != childnodes.end(); cnIt ++ ){
            const uint32_t contextID = *cnIt;
            if( !eventSnapshotContexts.empty() && eventSnapshotContexts.count( contextID ) == 1 ){
              // if the child is also a snapshot context, we need to look deeper
              //contextIDs.push( contextID );
            }else{
              // don't need to check for duplicate contexts
              appendCandidateContext( contextID );
            }
          }

        }
        */
    }
    void appendCandidateContext( const uint32_t contextID){
        TreeNode *node = new TreeNode( contextID ); // push_front() operation
        prev->next = node;
        node->prev = prev;
        node->next = NULL;
        ctxNodes[ contextID ] = node ; // map context id to the node address
        prev = node;
    }
    void unionContexts(const TreeNode& head){
      TreeNode *node = head.next;
      const uint32_t globalContextID = 1; //contextMapping.findIDByName( "" ); // find the id of the global context
      while( node != NULL ){
        if( node->contextID == globalContextID ){ 
          node = node->next;
          continue; 
        }// if it's not global context
        
        uint32_t parentContextID = node->contextID;

        TreeNode *next = node->next;
        while( (parentContextID = getParentContextID( parentContextID ) ) != 0  ){
          map< uint32_t, TreeNode*, SoftState >::iterator ancestor = ctxNodes.find( parentContextID );
          if( ancestor != ctxNodes.end() ){ // if its ancestor is in the list
            //TreeNode* ancestorNode = ancestor->second;
            // adjust linkage: remove the node from the list
            node->prev->next = node->next;
            if( node->next != NULL ){
              node->next->prev = node->prev;
            }

            //ancestorNode->addChild( node );
            break;
          }
        }
        node = next;

      }
    }
    uint32_t getParentContextID( const uint32_t contextID ) const{
      if( contextID == 1 ) { return 0; } // global context does not have parent
      return 0;
      //return contextMapping.getParentContextID( contextID );
    }

    map< uint32_t, TreeNode*, SoftState> ctxNodes;
    TreeNode* prev;
    list< uint32_t > cutSet;
    set< uint32_t > eventSnapshotContexts;
    const ContextMapping& contextMapping;
};

}
#endif
