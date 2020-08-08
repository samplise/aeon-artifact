/* 
 * BitmapDigest.cc : part of the Mace toolkit for building distributed systems
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
#include "BitmapDigest.h"
#include "Log.h"
#include "LogIdSet.h"
#include "mace-macros.h"
// #include "mace.h"
// #include "stdio.h"
// #include "math.h"

bitmap_digest::bitmap_digest(int size)
{
  ADD_FUNC_SELECTORS;
  if (size % 8 != 0)
    {
      maceError("size is not on multiple of 8, exiting: %d", size);
      exit(50);
    }
#if VARIABLE_BITMAP
  bm.tab = new unsigned char [size/8];
#endif
  bm.tab_size = size;
  bm.start_bit = 0;
  bm.start_seq = 0;
  bm.last_seq = 0;
  reset();
}

bitmap_digest::~bitmap_digest()
{
}

int bitmap_digest::which_byte(int seq)
{
  return (seq % bm.tab_size)/8;
}

int bitmap_digest::which_bit(int seq)
{
  return seq % 8;
}

int bitmap_digest::in_table(int seq)
{
  if (seq < bm.start_seq ||seq > bm.last_seq)
    return 0;
  return 1;
}

void bitmap_digest::set_bit(int seq, int set_value) 
{
  unsigned char value = 0x01;
  value = value<<which_bit(seq);
  int which= which_byte(seq);
//    printf("bitmap_digest::set_bit %d  %d %d %d\n", seq,set_value,which_byte(seq),which_bit(seq));
  if (set_value) 
    {
      bm.tab[which] = (bm.tab[which]|value);
    }
  else 
    {
      bm.tab[which] = (bm.tab[which]&(~value));
    }
}

bool bitmap_digest::contains(int seq) 
{
  if (! in_table(seq)) {
    return 0;
  }
  unsigned char value = bm.tab[ which_byte(seq)];
  value = value>>which_bit(seq);
  return (value&0x01);
}

int bitmap_digest::insert (int item, int suppress)
{
  ADD_FUNC_SELECTORS;
//  printf("bitmap_digest::insert: %d start %d last %d\n", item,bm.start_seq,bm.last_seq);

  if (contains(item)) {
    //    printf("%d already in bitmap\n", item);
    return 0;
  }
  if (item > bm.last_seq)
    bm.last_seq = item;
  if (bm.last_seq-bm.start_seq+1 > bm.tab_size) {
    int old_start = bm.start_seq;
    bm.start_seq = bm.last_seq-bm.tab_size+1;
    maceDebug(1,"bitmap_digest::insert bm.start_seq : %d", bm.start_seq );

    for (int i = old_start; i < bm.start_seq-1  ;i++ )
        {
          set_bit(i, 0);
        }
  }
  set_bit(item, 1);
  return bm.start_seq;
}

int bitmap_digest::remove_item (int item)
{
  if (! in_table( item)) {
    return 0;
  }
  set_bit(item, 0);
  return 1;
}

int bitmap_digest::get_lowest_sequence()
{
  return bm.start_seq;
}

int bitmap_digest::get_highest_sequence()
{
  return bm.last_seq;
}

// ---------------------------------------------- 
// size_compacted
// ---------------------------------------------- 

int  bitmap_digest::size_compacted_in_bytes ()
{
#if !VARIABLE_BITMAP
  return sizeof(bm);
#else
  return size_core_in_bytes ()+bm.tab_size/8;
#endif
}


// ---------------------------------------------- 
// serialize
// ---------------------------------------------- 

void  bitmap_digest::serialize (unsigned char* buffer)
{
#if !VARIABLE_BITMAP
  memmove(buffer, (unsigned char*)&bm,sizeof(bm));
#else
  memmove(buffer, (unsigned char*)&bm, size_core_in_bytes ());
  memmove(buffer+size_core_in_bytes (), bm.tab, bm.tab_size/8);
#endif
}

// ---------------------------------------------- 
// import
// ---------------------------------------------- 

void  bitmap_digest::import (unsigned char* buffer)
{
//  printf("bitmap_digest::import \n");
#if !VARIABLE_BITMAP
  memmove((unsigned char*)&bm, buffer, sizeof(bm));
#else
  int existing_size = bm.tab_size;
  memmove((unsigned char*)&bm, buffer, size_core_in_bytes ());
  if (bm.tab && existing_size != bm.tab_size)
    {
      delete bm.tab;
      bm.tab = NULL;
    }
 
  if (!bm.tab)
    {
      bm.tab = new unsigned char [bm.tab_size/8];
    }

  memmove(bm.tab, buffer+size_core_in_bytes (), bm.tab_size/8);
#endif
}

// ---------------------------------------------- 
// reset
// ---------------------------------------------- 

void  bitmap_digest::reset ()
{
  for (int i=0; i<bm.tab_size/8; i++) 
    {
      bm.tab[i] = 0;
    }
}
