#ifndef _EVENTEXTRAFIELD_H
#define _EVENTEXTRAFIELD_H

#include "Printable.h"
#include "Serializable.h"
#include "mstring.h"
#include "mset.h"
//#include "Event.h"

namespace mace{
  class __asyncExtraField : public mace::PrintPrintable, public mace::Serializable {
  private:
    mutable size_t serializedByteSize;
  public:
    mace::string targetContextID;
    mace::set<mace::string> snapshotContextIDs;
    int16_t eventType;
    int16_t lockingType; 
    mace::string methodName;

    mace::string get_targetContextID() const { return targetContextID; }
    mace::set<mace::string> get_snapshotContextIDs() const { return snapshotContextIDs; }
    
    __asyncExtraField() : targetContextID(), snapshotContextIDs(), lockingType(1), methodName() {}
    __asyncExtraField(mace::string const & _targetContextID ) : serializedByteSize(0), targetContextID(_targetContextID), methodName() {}
    __asyncExtraField(mace::string const & _targetContextID, mace::set<mace::string> const & _snapshotContextIDs /*,mace::Event const & _event,*/ /*bool const & _isRequest*/ ) : serializedByteSize(0), targetContextID(_targetContextID), snapshotContextIDs(_snapshotContextIDs) /*,event(_event),*/ /*isRequest(_isRequest)*/ {}
    virtual ~__asyncExtraField() {}
    
    void printNode(mace::PrintNode& __pr, const std::string& __name) const {
      mace::PrintNode ____asyncExtraFieldPrinter(__name, "__asyncExtraField");
      mace::printItem(____asyncExtraFieldPrinter, "targetContextID", &(targetContextID));;
      mace::printItem(____asyncExtraFieldPrinter, "snapshotContextIDs", &(snapshotContextIDs));;
      mace::printItem(____asyncExtraFieldPrinter, "eventType", &(eventType));;
      mace::printItem(____asyncExtraFieldPrinter, "lockingType", &(lockingType));
      __pr.addChild(____asyncExtraFieldPrinter);
    }
    void print(std::ostream& __out) const {
      __out << "__asyncExtraField("  ;
          __out << "targetContextID=";  mace::printItem(__out, &(targetContextID));
          __out << ", ";
          __out << "snapshotContextIDs=";  mace::printItem(__out, &(snapshotContextIDs));
          __out << ", ";
          __out << "eventType=";  mace::printItem(__out, &(eventType));
          __out << ", ";
          __out << "lockingType=";  mace::printItem(__out, &(lockingType));
      __out << ")";
    }
    void printState(std::ostream& __out) const {
      __out << "__asyncExtraField(" ;
          __out << "targetContextID=";  mace::printState(__out, &(targetContextID), (targetContextID));
          __out << ", ";
          __out << "snapshotContextIDs=";  mace::printState(__out, &(snapshotContextIDs), (snapshotContextIDs));
          __out << ", ";
          __out << "eventType=";  mace::printState(__out, &(eventType), (eventType));
          __out << ", ";
          __out << "lockingType=";  mace::printState(__out, &(lockingType), (lockingType));
          __out << ")";
    }
    void serialize(std::string& str) const {
      serializedByteSize = str.size();
      mace::serialize(str, &targetContextID);
      mace::serialize(str, &snapshotContextIDs);
      mace::serialize(str, &eventType);
      mace::serialize(str, &lockingType);
      mace::serialize(str, &methodName);
      serializedByteSize = str.size() - serializedByteSize;
    }
    int deserialize(std::istream& __mace_in) throw(mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &targetContextID);
      serializedByteSize +=  mace::deserialize(__mace_in, &snapshotContextIDs);
      serializedByteSize +=  mace::deserialize(__mace_in, &eventType);
      serializedByteSize +=  mace::deserialize(__mace_in, &lockingType);
      serializedByteSize +=  mace::deserialize(__mace_in, &methodName);
      return serializedByteSize;
    }
    
    size_t getSerializedSize() const {
      return serializedByteSize;
    }
    
  };

}
#endif
