/*****************************************************************************/
/*                                                                           */
/* Copyright (c) 2011-2015 Seoul National University.                        */
/* All rights reserved.                                                      */
/*                                                                           */
/* Redistribution and use in source and binary forms, with or without        */
/* modification, are permitted provided that the following conditions        */
/* are met:                                                                  */
/*   1. Redistributions of source code must retain the above copyright       */
/*      notice, this list of conditions and the following disclaimer.        */
/*   2. Redistributions in binary form must reproduce the above copyright    */
/*      notice, this list of conditions and the following disclaimer in the  */
/*      documentation and/or other materials provided with the distribution. */
/*   3. Neither the name of Seoul National University nor the names of its   */
/*      contributors may be used to endorse or promote products derived      */
/*      from this software without specific prior written permission.        */
/*                                                                           */
/* THIS SOFTWARE IS PROVIDED BY SEOUL NATIONAL UNIVERSITY "AS IS" AND ANY    */
/* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE    */
/* DISCLAIMED. IN NO EVENT SHALL SEOUL NATIONAL UNIVERSITY BE LIABLE FOR ANY */
/* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL        */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS   */
/* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)     */
/* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,       */
/* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN  */
/* ANY WAY OUT OF THE USE OF THIS  SOFTWARE, EVEN IF ADVISED OF THE          */
/* POSSIBILITY OF SUCH DAMAGE.                                               */
/*                                                                           */
/* Contact information:                                                      */
/*   Center for Manycore Programming                                         */
/*   Department of Computer Science and Engineering                          */
/*   Seoul National University, Seoul 08826, Korea                           */
/*   http://aces.snu.ac.kr                                                   */
/*                                                                           */
/* Contributors:                                                             */
/*   Jungwon Kim, Sangmin Seo, Gangwon Jo, Jun Lee, Jeongho Nah,             */
/*   Jungho Park, Junghyun Kim, and Jaejin Lee                               */
/*                                                                           */
/*****************************************************************************/

#ifndef __SNUCL__CL_OBJECT_H
#define __SNUCL__CL_OBJECT_H

#include "CLDispatch.h"

unsigned long CLObject_GetNewID();

template <typename st_obj_type, class c_obj_type>
class CLObject {
 public:
  CLObject() {
    id_ = CLObject_GetNewID();
    ref_cnt_ = 1;
    st_obj_.c_obj = (c_obj_type*)this;
    st_obj_.dispatch = CLDispatch::GetDispatch();
  }
  virtual ~CLObject() {}

  unsigned long id() const { return id_; }
  int ref_cnt() const { return ref_cnt_; }
  st_obj_type* st_obj() { return &st_obj_; }

  /*
   * The Cleanup() function is called immediately before the object is deleted.
   * This is used to avoid calling virtual functions in the destructor.
   */
  virtual void Cleanup() {}

  void Retain() {
    int cur_ref_cnt;
    do {
      cur_ref_cnt = ref_cnt_;
    } while (!__sync_bool_compare_and_swap(&ref_cnt_, cur_ref_cnt,
                                           cur_ref_cnt + 1));
  }

  void Release() {
    int cur_ref_cnt;
    do {
      cur_ref_cnt = ref_cnt_;
    } while (!__sync_bool_compare_and_swap(&ref_cnt_, cur_ref_cnt,
                                           cur_ref_cnt - 1));
    if (cur_ref_cnt == 1) {
      Cleanup();
      delete this;
    }
  }

 private:
  unsigned long id_;
  int ref_cnt_;
  st_obj_type st_obj_;
};

#endif // __SNUCL__CL_OBJECT_H
