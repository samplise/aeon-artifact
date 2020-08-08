/* 
 * ScopedSerialize.h : part of the Mace toolkit for building distributed systems
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
#ifndef __SCOPED_SERIALIZE_H
#define __SCOPED_SERIALIZE_H

#include "Serializable.h"
#include "Printable.h"
#include "mace-macros.h"
#include "params.h"

/**
 * \file ScopedSerialize.h
 * \brief declares the ScopedSerialize class.
 */

/**
 * \addtogroup Scoped
 * @{
 */

/**
 * \brief connects a string and object for serialization and re-deserialization.  
 *
 * Note, this only works right now for downcalls.  The equivalent upcall tool
 * does not exist that I am aware of.  What this class does is in its
 * constructor serialize an object into a string.  If the object passed in is
 * constant, that's all that happens.  But if the object is non-const, then on
 * destruction of this object, the string is deserialized back into the object.
 * This is to allow a higher level mace service to pass an object down, it
 * automatically be converted into a string, and then for the lower layer to
 * modify the string reference (replacing it with a new value for example), and
 * then deserializing the string on return.
 *
 * A reverse case would be to upcall a string, have it deserialize into an
 * object, then re-serialize it once the upcall returns.  That isn't written
 * yet (though one-way serialization works just fine).
 *
 * \todo move to mace namespace?
 */
template<typename STRING, typename ORIGIN>
class ScopedSerialize {
private:
  STRING& str;
  const ORIGIN* constObject;
  ORIGIN* object;
public:
  /// this constructor is used if the object is const.  This does a one-time serialization.
  ScopedSerialize(STRING& s, const ORIGIN& obj) : str(s), constObject(&obj), object(NULL) 
  {
    ADD_SELECTORS("ScopedSerialize");
    static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
				 params::get("LOG_SCOPED_SERIALIZE", false));
    str = mace::serialize(constObject);
    if (printSS) {
      maceout << HashString::hash(str) << " " << *constObject << Log::endl;
//       mace::printItem(maceout, constObject);
//       maceout << Log::endl;
    }
  }
  /// this constructor is used if the object is non-const.  This serializes in constructor, and deserializes on the destructor.
  ScopedSerialize(STRING& s, ORIGIN& obj) : str(s), constObject(&obj), object(&obj)
  {
    ADD_SELECTORS("ScopedSerialize");
    static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
				 params::get("LOG_SCOPED_SERIALIZE", false));
    str = mace::serialize(constObject);
    if (printSS) {
      maceout << HashString::hash(str) << " " << *constObject << Log::endl;
//       mace::printItem(maceout, constObject);
//       maceout << Log::endl;
    }
  }
  ~ScopedSerialize() {
    ADD_SELECTORS("ScopedSerialize");
    static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
				 params::get("LOG_SCOPED_SERIALIZE", false));
    if (object) {
      mace::deserialize(str, object);
      if (printSS) {
	maceout << HashString::hash(str) << " " << *object << Log::endl;
// 	mace::printItem(maceout, object);
// 	maceout << Log::endl;
      }
      object = NULL;
    }
    constObject = NULL;
  }
}; // class ScopedSerialize

template<typename STRING, typename ORIGIN>
class ScopedDeserialize {
private:
  STRING* str;
  const STRING* constStr;
  ORIGIN& object;
public:
  /// this constructor is used if the object is const.  This does a one-time serialization.
  ScopedDeserialize(const STRING& s, ORIGIN& obj) : str(0), constStr(&s), object(obj)
  {
    ADD_SELECTORS("ScopedDeserialize");
//     static const bool isSimulated = params::containsKey("MACE_SIMULATE_NODES");
    mace::deserialize(*constStr, &object);
//     if (isSimulated) {
//       maceout << HashString::hash(*constStr) << " " << object << Log::endl;
//     }
  }
  /// this constructor is used if the object is non-const.  This serializes in constructor, and deserializes on the destructor.
  ScopedDeserialize(STRING& s, ORIGIN& obj) : str(&s), constStr(&s), object(obj)
  {
    ADD_SELECTORS("ScopedDeserialize");
//     static const bool isSimulated = params::containsKey("MACE_SIMULATE_NODES");
    mace::deserialize(*constStr, &object);
//     if (isSimulated) {
//       maceout << HashString::hash(*constStr) << " " << object << Log::endl;
//     }
  }
  ~ScopedDeserialize() {
    ADD_SELECTORS("ScopedDeserialize");
//     static const bool isSimulated = params::containsKey("MACE_SIMULATE_NODES");
    if (str) {
      *str = mace::serialize(&object);
//       if (isSimulated) {
// 	maceout << HashString::hash(*str) << " " << *object << Log::endl;
//       }
    }
  }
}; // class ScopedDeserialize

/**
 * @}
 */
#include "mdeque.h" 
template<typename STRING,  typename ORIGIN>
class ScopedSerialize< mace::deque<STRING>,  mace::deque<ORIGIN> > {
private:
	typedef mace::deque<STRING> StringDeque;
	typedef mace::deque<ORIGIN> ObjectDeque;
	StringDeque& strQueue;
	const ObjectDeque* constObjectQueue;
	ObjectDeque* objectQueue;
public:
	/// this constructor is used if the object is const.  This does a one-time serialization.
	ScopedSerialize(StringDeque& s,  const ObjectDeque& obj) : strQueue(s),  constObjectQueue(&obj),  objectQueue(NULL){
		ADD_SELECTORS("ScopedSerialize");
    // chuangw: printSS not used?
		//static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
	//			params::get("LOG_SCOPED_SERIALIZE",  false));
		strQueue.clear();
		//strQueue.reserve(constObjectQueue->size());
		for (typename ObjectDeque::const_iterator i = constObjectQueue->begin(); i != constObjectQueue->end(); i++) {
			std::string str;
			mace::serialize(str, &(*i));
			strQueue.push_back(str);
			//if (printSS) {
			//  maceout << HashString::hash(str) << " " << *constObject << Log::endl;
			//     mace::printItem(maceout,  constObject);
			//  //     maceout << Log::endl;
			//}
		}
	}
	// this constructor is used if the object is non-const.  This serializes in constructor,  and deserializes on the destructor.
	ScopedSerialize(StringDeque& s,  ObjectDeque& obj) : strQueue(s),  constObjectQueue(&obj),  objectQueue(&obj)
	{
		ADD_SELECTORS("ScopedSerialize");
		/*  
		static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
				params::get("LOG_SCOPED_SERIALIZE",  false));
		*/

		strQueue.clear();
		//strList.reserve(constObjectList->size());
		for (typename ObjectDeque::const_iterator i = constObjectQueue->begin(); i != constObjectQueue->end(); i++) {
			std::string str;
			mace::serialize(str, &(*i));
			strQueue.push_back(str);
			//if (printSS) {
			//  maceout << HashString::hash(str) << " " << *constObject << Log::endl;
			//  //     mace::printItem(maceout,  constObject);
			//  //     maceout << Log::endl;
			//}
			}
	}
	  
	~ScopedSerialize() {
		ADD_SELECTORS("ScopedSerialize");
		/*  
		static const bool printSS = (params::containsKey("MACE_SIMULATE_NODES") ||
				params::get("LOG_SCOPED_SERIALIZE",  false));
		*/
		if (objectQueue) {
			//objectList->resize(strList.size());
			typename ObjectDeque::iterator j = objectQueue->begin();
			for (typename StringDeque::const_iterator i = strQueue.begin(); i != strQueue.end(); i++) {
				mace::deserialize(*i,  &(*j));
				j++;
				//if (printSS) {
				//  maceout << HashString::hash(str) << " " << *object << Log::endl;
				//  //  mace::printItem(maceout,  object);
				//  //  maceout << Log::endl;
				//}
			}
			objectQueue = NULL;
	  }
		constObjectQueue = NULL;
	}
}; // class ScopedSerialize

template<typename STRING,  typename ORIGIN>
class ScopedDeserialize<mace::deque<STRING>,  mace::deque<ORIGIN> > {
private:
	typedef mace::deque<STRING> StringDeque;
	typedef mace::deque<ORIGIN> ObjectDeque;
	StringDeque* strQueue;
	const StringDeque* constStrQueue;
	ObjectDeque& objectQueue;

public:
  /// this constructor is used if the object is const.  This does a one-time serialization.
	ScopedDeserialize(const StringDeque& s,  ObjectDeque& obj) : strQueue(0),  constStrQueue(&s),  objectQueue(obj)
	{
		//objectList.resize(constStrList->size());
		typename mace::deque<ORIGIN>::iterator j = objectQueue.begin();
		for (typename StringDeque::const_iterator i = constStrQueue->begin(); i != constStrQueue->end(); i++) {
			mace::deserialize(*i,  &(*j));
			j++;
		}
	}
	
	/// this constructor is used if the object is non-const.  This serializes in constructor,  and deserializes on the destructor.
	ScopedDeserialize(StringDeque& s,  ObjectDeque& obj) : strQueue(&s),  constStrQueue(&s),  objectQueue(obj)
	{
		//objectList.resize(constStrList->size());
		typename mace::deque<ORIGIN>::iterator j = objectQueue.begin();
		for (typename StringDeque::const_iterator i = constStrQueue->begin(); i != constStrQueue->end(); i++) {
			mace::deserialize(*i,  &(*j));
			j++;
		}
	}
	
	~ScopedDeserialize() {
		if (strQueue) {
			//strQueue->resize(objectList.size());
			typename StringDeque::iterator j = strQueue->begin();
			for (typename ObjectDeque::const_iterator i = objectQueue.begin(); i != objectQueue.end(); i++) {
				mace::serialize(*j,  &(*i));
				j++;
			}
		  strQueue = NULL;
		}
		constStrQueue = NULL;
	}
}; // class ScopedSerialize



#endif //__SCOPED_SERIALIZE_H
