/* 
 * BitmapDigest.h : part of the Mace toolkit for building distributed systems
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
#include "Digest.h"

/**
 * \file BitmapDigest.h
 * \brief defines the bitmap_digest class
 */

#ifndef __bitmap_digest
#define __bitmap_digest
//const static int BITMAP_SIZE = 1024*22;
/// size of the bitmap in bits or items.
const static int BITMAP_SIZE = 1024*6; //was *4

/**
 * \brief struct providing the storage for the bitmap_digest
 */
typedef struct bitmapy {
  int tab_size; ///< size of the digest
  int start_bit; ///< the bit position offset into the bitmap
  int start_seq; ///< first index in the digest represented
  int last_seq; ///< the last sequence set.
#if VARIABLE_BITMAP
  unsigned char* tab; ///< the bitmap itself
  bitmapy():tab( 0) {}
  int  size_core_in_bytes (){sizeof(*this)-sizeof(unsigned char* );};
#else
  unsigned char tab[BITMAP_SIZE/8];  ///< the bitmap itself
#endif
} flat_bitmap;

/**
 * \addtogroup Digests
 * @{
 */

/**
 * \brief a correct and complete digest over the range it is currently supported.
 *
 * The range updates automatically to handle new inserts.  As new inserts are
 * added to the bitmap, the range is rolled forward to include the new value in
 * the range.  Earlier entries are zeroed out and forgotten.  A bit position
 * can always be computed as the modulus in the size of the digest.
 */
class bitmap_digest : public digest
	{
	
public:
 bitmap_digest	(int size = BITMAP_SIZE);
~bitmap_digest	( );
public:
  bool  contains (int item);
  //insert returns the lowest sequence number remaining in the  digest even after truncation
  int  insert (int item, int suppress = 1);
  //returns 1 if it actually cleared the bit (i.e. it was in range)
  int  remove_item (int item);
  void   serialize (unsigned char* buffer);
  void  import (unsigned char* buffer);
  int  size_compacted_in_bytes ();
  /*   int  reset (); */
  void  reset ();
  int  get_lowest_sequence ();
  int  get_highest_sequence ();

  flat_bitmap bm; ///< the actual storage of the bitmap
 protected:
  int in_table (int seq);
  int  which_byte  (int seq);
  int  which_bit  (int seq);
  void set_bit (int seq, int set_value);

 private:
 	};
/** @} */
#endif //__bitmap_digest
