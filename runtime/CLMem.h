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

#ifndef __SNUCL__CL_MEM_H
#define __SNUCL__CL_MEM_H

#include <map>
#include <set>
#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

#define LATEST_HOST ((CLDevice*)1)

class CLContext;
class CLDevice;
class MemObjectDestructorCallback;

typedef struct _CLMapWritebackLayout {
  size_t origin[3];
  size_t region[3];
} CLMapWritebackLayout;

class CLMem: public CLObject<struct _cl_mem, CLMem> {
 private:
  CLMem(CLContext* context);

 public:
  virtual void Cleanup();
  virtual ~CLMem();

  CLContext* context() const { return context_; }
  cl_mem_object_type type() const { return type_; }
  cl_mem_flags flags() const { return flags_; }
  size_t size() const { return size_; }
  CLMem* parent() const { return parent_; }
  size_t offset() const { return offset_; }

  cl_image_format image_format() const { return image_format_; }
  cl_image_desc image_desc() const { return image_desc_; }
  size_t image_width() const { return image_desc_.image_width; }
  size_t image_height() const { return image_desc_.image_height; }
  size_t image_depth() const { return image_desc_.image_depth; }
  size_t image_element_size() const { return image_element_size_; }
  size_t image_channels() const { return image_channels_; }
  size_t image_row_pitch() const { return image_row_pitch_; }
  size_t image_slice_pitch() const { return image_slice_pitch_; }

  cl_int GetMemObjectInfo(cl_mem_info param_name, size_t param_value_size,
                          void* param_value, size_t* param_value_size_ret);
  cl_int GetImageInfo(cl_image_info param_name, size_t param_value_size,
                      void* param_value, size_t* param_value_size_ret);

  bool IsBuffer() const { return type_ == CL_MEM_OBJECT_BUFFER; }
  bool IsImage() const { return type_ != CL_MEM_OBJECT_BUFFER; }
  bool IsSubBuffer() const { return parent_ != NULL; }
  bool IsWithinRange(size_t offset, size_t cb) const;
  bool IsWritable() const;
  bool IsHostReadable() const;
  bool IsHostWritable() const;

  size_t* GetImageRegion();
  size_t GetRegionSize(const size_t* region) const;

  bool IsValidChildFlags(cl_mem_flags child_flags) const;
  void InheritFlags(cl_mem_flags& child_flags) const;

  void* GetHostPtr() const;
  void AllocHostPtr();

  bool HasDevSpecific(CLDevice* device);
  void* GetDevSpecific(CLDevice* device);

  bool EmptyLatest();
  bool HasLatest(CLDevice* device);
  CLDevice* FrontLatest();
  void AddLatest(CLDevice* device);
  void SetLatest(CLDevice* device);
  CLDevice* GetNearestLatest(CLDevice* device);

  void* MapAsBuffer(cl_map_flags map_flags, size_t offset, size_t size);
  void* MapAsImage(cl_map_flags map_flags, const size_t* origin,
                   const size_t* region, size_t* image_row_pitch,
                   size_t* image_slice_pitch);
  bool IsMapInitRequired(void* ptr);
  bool GetMapWritebackLayoutForBuffer(void* ptr, size_t* offset, size_t* size);
  bool GetMapWritebackLayoutForImage(void* ptr, size_t* origin,
                                     size_t* region);
  void Unmap(void* ptr);

  void AddDestructorCallback(MemObjectDestructorCallback* callback);

 private:
  void SetHostPtr(void* host_ptr);

 private:
  CLContext* context_;
  cl_mem_object_type type_;
  cl_mem_flags flags_;
  size_t size_;

  CLMem* parent_;
  size_t offset_;

  void* host_ptr_;
  bool alloc_host_;
  bool use_host_;

  cl_image_format image_format_;
  cl_image_desc image_desc_;
  size_t image_element_size_;
  size_t image_channels_;
  size_t image_row_pitch_;
  size_t image_slice_pitch_;
  size_t image_region_[3];

  std::map<CLDevice*, void*> dev_specific_;
  std::set<CLDevice*> dev_latest_;
  std::vector<MemObjectDestructorCallback*> callbacks_;

  cl_uint map_count_;
  std::map<void*, CLMapWritebackLayout> map_writeback_;

  pthread_mutex_t mutex_dev_specific_;
  pthread_mutex_t mutex_dev_latest_;
  pthread_mutex_t mutex_host_ptr_;
  pthread_mutex_t mutex_map_;
  pthread_mutex_t mutex_callbacks_;

 public:
  static CLMem* CreateBuffer(CLContext* context, cl_mem_flags flags,
                             size_t size, void* host_ptr, cl_int* err);
  static CLMem* CreateSubBuffer(CLMem* parent, cl_mem_flags flags,
                                size_t origin, size_t size, cl_int* err);
  static CLMem* CreateImage(CLContext* context, cl_mem_flags flags,
                            const cl_image_format* image_format,
                            const cl_image_desc* image_desc, void* host_ptr,
                            cl_int* err);

 private:
  static void InterpretImageFormat(const cl_image_format* image_format,
                                   size_t& element_size, size_t& channels);
};

#endif // __SNUCL__CL_MEM_H
