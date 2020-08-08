/* 
 * Digest.h : part of the Mace toolkit for building distributed systems
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
#ifndef __digest
#define __digest

/**
 * \file Digest.h
 * \brief defines the base class (interface) for the digest classes.
 */

/**
 * \addtogroup Digests
 * \brief Digests support summaries to tell whether an index has been inserted or not.
 *
 * All digest classes support inserting indexes, then checking whether they
 * have been inserted.  They need not be perfect, and can instead be used as a
 * heuristic for efficiency sake in the network.  Some may however be perfect.
 * Each should document its properties.
 * 
 * @{
 */

/// the digest interface, no default implementations
class digest
{
	
public:
  digest ( );
  virtual ~digest ();
public:
  virtual bool  contains (int item) = 0; ///< check to see if an item is in the digest
  /**
   * \brief inserts an item
   * @param item the item to insert
   * @param suppress when feasible, tells the filter whether to suppress dropping values
   * @return returning the lowest sequence number remaining in the digest (even after truncation)
   */
  virtual int  insert (int item, int suppress = 1) = 0; 
  /**
   * \brief serializes the digest into the buffer
   * \warning if the digest is to be continue being used, this should be replaced with basic mace serialization.
   */
  virtual void serialize (unsigned char* buffer) = 0; 
  /**
   * \brief deserializes the buffer into the digest
   * \warning if the digest is to be continue being used, this should be replaced with basic mace serialization.
   */
  virtual void import (unsigned char* buffer) = 0;
  virtual int  size_compacted_in_bytes () = 0; ///< returns the size of the digest in bytes (for serialization)
  /*   virtual int  reset () = 0; */
  virtual void  reset () = 0; ///< resets the digest to empty
  virtual int  get_lowest_sequence () = 0; ///< returns the lowest sequence represented (yes or no) in the digest
 protected:
 private:
};

/** @} */

#endif //__digest
