#ifndef __ACCESSLINE_H

#define __ACCESSLINE_H

#include "ContextMapping.h"
#include "Event.h"
#include "ThreadStructure.h"
/**
 * \file AccessLine.h
 * \brief declares the AccessLine class
 */
namespace mace{
/**
 * \brief AccessLine checks if the current event is allowed to enter the context
 *
 * */
class AccessLine{
/**
 * \brief This function checks if the current event is allowed to enter the context
 * 
 * \param contextID Context ID requested to enter
 * \param currentMapping the current context mapping relation
 * */
public:
  AccessLine( const uint8_t serviceID, const uint32_t targetContextID, const ContextMapping& currentMapping ) {
    if( !granted( serviceID, targetContextID, currentMapping ) ){
      failStop(targetContextID);
    }
  }

  /**
   * \brief The event can only access a context if the access line is above it.
   *
   * @param serviceID the numerical ID of the context
   * @param targetContextID the numerical ID of the context
   * @param currentMapping the mapping object of the service
   * */
  static bool granted( const uint8_t serviceID, const uint32_t targetContextID, const ContextMapping& currentMapping ){
    ADD_SELECTORS("AccessLine::(constructor)");
    /*
    Event::EventSnapshotContextType const& snapshots = ThreadStructure::getEventSnapshotContexts();
    Event::EventSnapshotContextType::const_iterator sctxsIt;
    if( !snapshots.empty() && (sctxsIt = snapshots.find( serviceID ) ) != snapshots.end() ){
      const Event::EventServiceSnapshotContextType& snapshotContexts = sctxsIt->second;
      if( !snapshotContexts.empty() ){

        // if the target context has already been released. error
        for( Event::EventServiceSnapshotContextType::const_iterator sctxIt = snapshotContexts.begin(); sctxIt != snapshotContexts.end(); sctxIt++ ){
          uint32_t ctxID = sctxIt->first;
          if( ctxID == targetContextID ){
            macedbg(1)<< "returning false because target context " << targetContextID << " is already downgraded."<<Log::endl;
            return false;
          }
          // if the target context is the ancestor of any snapshot context, error
          uint32_t traverseID = ctxID;
          while( traverseID != 1 ){ // if not global context
            uint32_t parent = currentMapping.getParentContextID( traverseID );
            if( parent == targetContextID ){
              macedbg(1)<< "returning false because target context " << targetContextID << " is the parent of a already downgraded context "<< ctxID << "."<<Log::endl;
              return false;
            }
            traverseID = parent;

          }
        }
      }

    }
    // if the target context is the child/offspring context of any of the context that the event currently owns, return true
    Event::EventContextType const& allContexts = ThreadStructure::getEventContexts();
    Event::EventContextType::const_iterator ctxsIt;
    if( !allContexts.empty() && (ctxsIt = allContexts.find( serviceID ) ) != allContexts.end() ){
      const Event::EventServiceContextType& contexts = ctxsIt->second;
      if( !contexts.empty() ){

        // if the target is currently being owned, return true
        if( contexts.find( targetContextID ) != contexts.end() ){
          return true;
        }
        // if any currently owned context is the ancestor of the target context, return true
        uint32_t ancestorID = targetContextID;
        while( ancestorID != 1 ){
          uint32_t parentID = currentMapping.getParentContextID( ancestorID );
          if( contexts.find( parentID ) != contexts.end() ){
            return true;
          }
          ancestorID = parentID;
        }
        macedbg(1)<< "context: " << contexts << Log::endl;
        return false;
      }
    }
    */
    // if this event did not ever enter any contexts, it can enter any contexts.
    return true;
    
  }

  /**
   * determine if it's allowed to downgrade to a specific context
   * @param serviceID the numerical ID of the service
   * @param targetContextID the numerical ID of the context
   * @param the mapping of the service
   *
   * @return TRUE if it's allowed
   * */
  static bool checkDowngradeContext( const uint8_t serviceID, const uint32_t targetContextID, const ContextMapping& currentMapping ){
    ADD_SELECTORS("AccessLine::checkDowngradeContext");

    /*
    uint32_t parent = currentMapping.getParentContextID( targetContextID );
    //mace::string parentContextName = ContextMapping::getNameByID( currentMapping, parent);

    if( ThreadStructure::getCurrentServiceEventContexts().find( parent ) != ThreadStructure::getEventContexts( ).find( serviceID )->second.end() ){

    }
    */
    return true;
  }
private:
  void failStop(const uint32_t targetContextID){
    ADD_SELECTORS("AccessLine::failStop");
    // Entering a context c is allowed if the event already holds the lock of context c, or if c is the child context of one of the contexts this event currently holds.
    // In other words, the write line is above c.
    maceerr<<"invalid context transition. Requested context ID = "<<targetContextID;

    ABORT( "STOP" );
  }
private:
  //const uint8_t serviceID;
};

}
#endif
