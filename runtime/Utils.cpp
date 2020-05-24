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

#include "Utils.h"
#include <stdio.h>

LockFreeQueue::LockFreeQueue(unsigned long size) {
  size_ = size;
  idx_r_ = 0;
  idx_w_ = 0;
  elements_ = (volatile CLCommand**)(new CLCommand*[size]);
}

LockFreeQueue::~LockFreeQueue() {
  delete[] elements_;
}

bool LockFreeQueue::Enqueue(CLCommand* element) {
  unsigned long next_idx_w = (idx_w_ + 1) % size_;
  if (next_idx_w == idx_r_) return false;
  elements_[idx_w_] = element;
  __sync_synchronize();
  idx_w_ = next_idx_w;
  return true;
}

bool LockFreeQueue::Dequeue(CLCommand** element) {
  if (idx_r_ == idx_w_) return false;
  unsigned long next_idx_r = (idx_r_ + 1) % size_;
  *element = (CLCommand*) elements_[idx_r_];
  idx_r_ = next_idx_r;
  return true;
}

bool LockFreeQueue::Peek(CLCommand** element) {
  if (idx_r_ == idx_w_) return false;
  *element = (CLCommand*) elements_[idx_r_];
  return true;
}

unsigned long LockFreeQueue::Size() {
  if (idx_w_ >= idx_r_) return idx_w_ - idx_r_;
  return size_ - idx_r_ + idx_w_;
}

LockFreeQueueMS::LockFreeQueueMS(unsigned long size)
    : LockFreeQueue(size) {
  idx_w_cas_ = 0;
}

LockFreeQueueMS::~LockFreeQueueMS() {
}

bool LockFreeQueueMS::Enqueue(CLCommand* element) {
  while (true) {
    unsigned long prev_idx_w = idx_w_cas_;
    unsigned long next_idx_w = (prev_idx_w + 1) % size_;
    if (next_idx_w == idx_r_) return false;
    if (__sync_bool_compare_and_swap(&idx_w_cas_, prev_idx_w, next_idx_w)) {
      elements_[prev_idx_w] = element;
      while (!__sync_bool_compare_and_swap(&idx_w_, prev_idx_w, next_idx_w)) {}
      break;
    }
  }
  return true;
}

size_t PipeRead(const char* filename, char* buf, size_t size) {
  FILE* fp = popen(filename, "r");
  size_t read_size = fread(buf, sizeof(char), size, fp);
  pclose(fp);
  return read_size;
}

void CopyRegion(void* src, void* dst, size_t dimension,
                const size_t* src_origin, const size_t* dst_origin,
                const size_t* region, size_t element_size,
                size_t src_row_pitch, size_t src_slice_pitch,
                size_t dst_row_pitch, size_t dst_slice_pitch) {
  if (src_row_pitch == 0)
    src_row_pitch = region[0] * element_size;
  if (src_slice_pitch == 0)
    src_slice_pitch = region[1] * src_row_pitch;
  if (dst_row_pitch == 0)
    dst_row_pitch = region[0] * element_size;
  if (dst_slice_pitch == 0)
    dst_slice_pitch = region[1] * dst_row_pitch;

  if (dimension >= 3 && region[2] > 0 && region[1] > 0) {
    size_t sz = src_origin[2], dz = dst_origin[2];
    for (size_t z = 0; z < region[2]; z++, sz++, dz++) {
      size_t sy = src_origin[1], dy = dst_origin[1];
      for (size_t y = 0; y < region[1]; y++, sy++, dy++) {
        size_t soff = sz * src_slice_pitch + sy * src_row_pitch +
                      src_origin[0] * element_size;
        size_t doff = dz * dst_slice_pitch + dy * dst_row_pitch +
                      dst_origin[0] * element_size;
        memcpy((char*)dst + doff, (char*)src + soff, region[0] * element_size);
      }
    }
  } else if (dimension >= 2 && region[1] > 0) {
    size_t sy = src_origin[1], dy = dst_origin[1];
    for (size_t y = 0; y < region[1]; y++, sy++, dy++) {
      size_t soff = sy * src_row_pitch + src_origin[0] * element_size;
      size_t doff = dy * dst_row_pitch + dst_origin[0] * element_size;
      memcpy((char*)dst + doff, (char*)src + soff, region[0] * element_size);
    }
  } else {
    size_t soff = src_origin[0] * element_size;
    size_t doff = dst_origin[0] * element_size;
    memcpy((char*)dst + doff, (char*)src + soff, region[0] * element_size);
  }
}
