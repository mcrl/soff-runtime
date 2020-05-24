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

#ifndef __SNUCL__UTILS_H
#define __SNUCL__UTILS_H

#include <cstring>
#include <stddef.h>

#ifdef SNUCL_DEBUG
// Modified in soff-runtime
#include <stdio.h>
#define SNUCL_ERROR(fmt, ...)  fprintf(stdout, "* ERR * [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define SNUCL_INFO(fmt, ...)   fprintf(stdout, "* INF * [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define SNUCL_ERROR(fmt, ...)
#define SNUCL_INFO(fmt, ...)
#endif

#define GET_OBJECT_INFO(param, type, value)                 \
  case param: {                                             \
    size_t size = sizeof(type);                             \
    if (param_value) {                                      \
      if (param_value_size < size) return CL_INVALID_VALUE; \
      memcpy(param_value, &(value), size);                  \
    }                                                       \
    if (param_value_size_ret) *param_value_size_ret = size; \
    break;                                                  \
  }

#define GET_OBJECT_INFO_T(param, type, value)               \
  case param: {                                             \
    size_t size = sizeof(type);                             \
    if (param_value) {                                      \
      if (param_value_size < size) return CL_INVALID_VALUE; \
      type temp = value;                                    \
      memcpy(param_value, &temp, size);                     \
    }                                                       \
    if (param_value_size_ret) *param_value_size_ret = size; \
    break;                                                  \
  }

#define GET_OBJECT_INFO_A(param, type, value, length)       \
  case param: {                                             \
    size_t size = sizeof(type) * length;                    \
    if (value == NULL) {                                    \
      if (param_value_size_ret) *param_value_size_ret = 0;  \
      break;                                                \
    }                                                       \
    if (param_value) {                                      \
      if (param_value_size < size) return CL_INVALID_VALUE; \
      memcpy(param_value, value, size);                     \
    }                                                       \
    if (param_value_size_ret) *param_value_size_ret = size; \
    break;                                                  \
  }

#define GET_OBJECT_INFO_S(param, value)                     \
  case param: {                                             \
    if (value == NULL) {                                    \
      if (param_value_size_ret) *param_value_size_ret = 0;  \
      break;                                                \
    }                                                       \
    size_t size = sizeof(char) * (strlen(value) + 1);       \
    if (param_value) {                                      \
      if (param_value_size < size) return CL_INVALID_VALUE; \
      memcpy(param_value, value, size);                     \
    }                                                       \
    if (param_value_size_ret) *param_value_size_ret = size; \
    break;                                                  \
  }

class CLCommand;

// Single Producer & Single Consumer
class LockFreeQueue {
 public:
  LockFreeQueue(unsigned long size);
  virtual ~LockFreeQueue();

  virtual bool Enqueue(CLCommand* element);
  bool Dequeue(CLCommand** element);
  bool Peek(CLCommand** element);
  unsigned long Size();

protected:
  unsigned long size_;
  volatile CLCommand** elements_;
  volatile unsigned long idx_r_;
  volatile unsigned long idx_w_;
};

// Multiple Producers & Single Consumer
class LockFreeQueueMS: public LockFreeQueue {
 public:
  LockFreeQueueMS(unsigned long size);
  ~LockFreeQueueMS();

  bool Enqueue(CLCommand* element);

 private:
  volatile unsigned long idx_w_cas_;
};

size_t PipeRead(const char* filename, char* buf, size_t size);

void CopyRegion(void* src, void* dst, size_t dimension,
                const size_t* src_origin, const size_t* dst_origin,
                const size_t* region, size_t element_size,
                size_t src_row_pitch, size_t src_slice_pitch,
                size_t dst_row_pitch, size_t dst_slice_pitch);

#endif // __SNUCL__UTILS_H
