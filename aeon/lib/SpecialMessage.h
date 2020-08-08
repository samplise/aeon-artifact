#include "Message.h"
#include "Event.h"
#include "EventExtraField.h"
#ifndef _SPECIALMESSAGE_H_
#define _SPECIALMESSAGE_H_

#include <boost/shared_ptr.hpp>
//#define INTERNALMESSAGE_USE_SHARED_PTR
namespace mace{

class InternalMessageHelper : public Message{ //public Serializable, virtual public Printable  {
public:

  virtual uint8_t getType() const{
    return std::numeric_limits<uint8_t>::max();
  }
  
};
#ifdef INTERNALMESSAGE_USE_SHARED_PTR
  typedef boost::shared_ptr<InternalMessageHelper> InternalMessageHelperPtr ;
#else
  typedef InternalMessageHelper* InternalMessageHelperPtr;
#endif

class AsyncEvent_Message: public InternalMessageHelper{
private:
protected:

  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
public:
  virtual __asyncExtraField & getExtra() = 0;
  virtual mace::Event & getEvent() = 0;

  size_t getSerializedSize() const {
    if (serializedByteSize == 0 && serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedByteSize;
  }
  std::string serializeStr() const {
    if (serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedCache;
  }
  void deserializeStr(const std::string& __s) throw (mace::SerializationException) {
    serializedCache = __s;
    Serializable::deserializeStr(__s);
  }
};

class ApplicationUpcall_Message: public InternalMessageHelper{
private:
protected:

  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
public:

  size_t getSerializedSize() const {
    if (serializedByteSize == 0 && serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedByteSize;
  }
  std::string serializeStr() const {
    if (serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedCache;
  }
  void deserializeStr(const std::string& __s) throw (mace::SerializationException) {
    serializedCache = __s;
    Serializable::deserializeStr(__s);
  }
};
class Routine_Message: public InternalMessageHelper{
private:
protected:

  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
public:
  virtual const uint32_t getTargetContextID() = 0;
  virtual mace::Event & getEvent() = 0;
  
  size_t getSerializedSize() const {
    if (serializedByteSize == 0 && serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedByteSize;
  }
  std::string serializeStr() const {
    if (serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedCache;
  }
  void deserializeStr(const std::string& __s) throw (mace::SerializationException) {
    serializedCache = __s;
    Serializable::deserializeStr(__s);
  }
};
class Transition_Message: public InternalMessageHelper{
private:
protected:

  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
public:
  virtual const uint32_t getTargetContextID() = 0;
  virtual mace::Event & getEvent() = 0;

  size_t getSerializedSize() const {
    if (serializedByteSize == 0 && serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedByteSize;
  }
  std::string serializeStr() const {
    if (serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedCache;
  }
  void deserializeStr(const std::string& __s) throw (mace::SerializationException) {
    serializedCache = __s;
    Serializable::deserializeStr(__s);
  }
};
}
#endif
