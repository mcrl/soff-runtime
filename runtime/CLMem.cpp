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

#include "CLMem.h"
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <malloc.h>
#include <pthread.h>
#include <CL/cl.h>
#include "Callbacks.h"
#include "CLContext.h"
#include "CLDevice.h"
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLMem::CLMem(CLContext* context) {
  context_ = context;
  context_->Retain();
  context_->AddMem(this);

  parent_ = NULL;
  host_ptr_ = NULL;
  alloc_host_ = use_host_ = false;
  map_count_ = 0;

  pthread_mutex_init(&mutex_dev_specific_, NULL);
  pthread_mutex_init(&mutex_dev_latest_, NULL);
  pthread_mutex_init(&mutex_host_ptr_, NULL);
  pthread_mutex_init(&mutex_map_, NULL);
  pthread_mutex_init(&mutex_callbacks_, NULL);
}

void CLMem::Cleanup() {
  for (vector<MemObjectDestructorCallback*>::iterator it = callbacks_.begin();
       it != callbacks_.end();
       ++it) {
    (*it)->run(st_obj());
    free(*it);
  }
  for (map<CLDevice*, void*>::iterator it = dev_specific_.begin();
       it != dev_specific_.end();
       ++it) {
    (it->first)->FreeMem(this, it->second);
  }
  context_->RemoveMem(this);
}

CLMem::~CLMem() {
  if (alloc_host_) free(host_ptr_);
  if (parent_) parent_->Release();
  context_->Release();

  pthread_mutex_destroy(&mutex_dev_specific_);
  pthread_mutex_destroy(&mutex_dev_latest_);
  pthread_mutex_destroy(&mutex_host_ptr_);
  pthread_mutex_destroy(&mutex_map_);
  pthread_mutex_destroy(&mutex_callbacks_);
}

cl_int CLMem::GetMemObjectInfo(cl_mem_info param_name, size_t param_value_size,
                               void* param_value,
                               size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO(CL_MEM_TYPE, cl_mem_object_type, type_);
    GET_OBJECT_INFO(CL_MEM_FLAGS, cl_mem_flags, flags_);
    GET_OBJECT_INFO(CL_MEM_SIZE, size_t, size_);
    GET_OBJECT_INFO(CL_MEM_HOST_PTR, void*, host_ptr_);
    GET_OBJECT_INFO(CL_MEM_MAP_COUNT, cl_uint, map_count_);
    GET_OBJECT_INFO_T(CL_MEM_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO_T(CL_MEM_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO_T(CL_MEM_ASSOCIATED_MEMOBJECT, cl_mem,
                      (parent_ == NULL ? NULL : parent_->st_obj()));
    GET_OBJECT_INFO(CL_MEM_OFFSET, size_t, offset_);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLMem::GetImageInfo(cl_image_info param_name, size_t param_value_size,
                           void* param_value, size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO(CL_IMAGE_FORMAT, cl_image_format, image_format_);
    GET_OBJECT_INFO(CL_IMAGE_ELEMENT_SIZE, size_t, image_element_size_);
    GET_OBJECT_INFO(CL_IMAGE_ROW_PITCH, size_t, image_row_pitch_);
    GET_OBJECT_INFO(CL_IMAGE_SLICE_PITCH, size_t, image_slice_pitch_);
    GET_OBJECT_INFO(CL_IMAGE_WIDTH, size_t, image_desc_.image_width);
    GET_OBJECT_INFO(CL_IMAGE_HEIGHT, size_t, image_desc_.image_height);
    GET_OBJECT_INFO(CL_IMAGE_DEPTH, size_t, image_desc_.image_depth);
    GET_OBJECT_INFO(CL_IMAGE_ARRAY_SIZE, size_t, image_desc_.image_array_size);
    GET_OBJECT_INFO_T(CL_IMAGE_BUFFER, cl_mem, NULL);
    GET_OBJECT_INFO(CL_IMAGE_NUM_MIP_LEVELS, cl_uint,
                    image_desc_.num_mip_levels);
    GET_OBJECT_INFO(CL_IMAGE_NUM_SAMPLES, cl_uint, image_desc_.num_samples);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

bool CLMem::IsWithinRange(size_t offset, size_t cb) const {
  return (offset < size_ && (offset + cb) <= size_);
}

bool CLMem::IsWritable() const {
  return (flags_ & (CL_MEM_WRITE_ONLY | CL_MEM_READ_WRITE));
}

bool CLMem::IsHostReadable() const {
  return !(flags_ & (CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS));
}

bool CLMem::IsHostWritable() const {
  return !(flags_ & (CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS));
}

size_t* CLMem::GetImageRegion() {
  return image_region_;
}

size_t CLMem::GetRegionSize(const size_t* region) const {
  if (IsImage()) {
    return region[0] * region[1] * region[2] * image_element_size_;
  } else {
    return region[0] * region[1] * region[2];
  }
}

bool CLMem::IsValidChildFlags(cl_mem_flags child_flags) const {
  if ((flags_ & CL_MEM_WRITE_ONLY) &&
      (child_flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY)))
    return false;
  if ((flags_ & CL_MEM_READ_ONLY) &&
      (child_flags & (CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY)))
    return false;
  if ((flags_ & CL_MEM_HOST_WRITE_ONLY) &&
      (child_flags & CL_MEM_HOST_READ_ONLY))
    return false;
  if (child_flags & (CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                     CL_MEM_COPY_HOST_PTR))
    return false;
  if ((flags_ & CL_MEM_HOST_READ_ONLY) &&
      (child_flags & CL_MEM_HOST_WRITE_ONLY))
    return false;
  if ((flags_ & CL_MEM_HOST_NO_ACCESS) &&
      (child_flags & (CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)))
    return false;
  return true;
}

void CLMem::InheritFlags(cl_mem_flags& child_flags) const {
  if (!(child_flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY |
                       CL_MEM_WRITE_ONLY)))
    child_flags |= flags_ & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY |
                             CL_MEM_WRITE_ONLY);
  child_flags |= flags_ & (CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                           CL_MEM_COPY_HOST_PTR);
  if (!(child_flags & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY |
                       CL_MEM_HOST_WRITE_ONLY)))
    child_flags |= flags_ & (CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY |
                             CL_MEM_HOST_WRITE_ONLY);
}

void* CLMem::GetHostPtr() const {
  return host_ptr_;
}

void CLMem::AllocHostPtr() {
  if (host_ptr_ == NULL) {
    pthread_mutex_lock(&mutex_host_ptr_);
    if (host_ptr_ == NULL) {
      if (IsSubBuffer()) {
        parent_->AllocHostPtr();
        host_ptr_ = (void*)((size_t)parent_->host_ptr_ + offset_);
      } else {
        host_ptr_ = memalign(4096, size_);
        alloc_host_ = true;
      }
    }
    pthread_mutex_unlock(&mutex_host_ptr_);
  }
}

bool CLMem::HasDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  bool alloc = (dev_specific_.count(device) > 0);
  pthread_mutex_unlock(&mutex_dev_specific_);
  return alloc;
}

void* CLMem::GetDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  void* dev_specific;
  if (dev_specific_.count(device) > 0) {
    dev_specific = dev_specific_[device];
  } else {
    pthread_mutex_unlock(&mutex_dev_specific_);
    dev_specific = device->AllocMem(this);
    pthread_mutex_lock(&mutex_dev_specific_);
    dev_specific_[device] = dev_specific;
  }
  pthread_mutex_unlock(&mutex_dev_specific_);
  return dev_specific;
}

bool CLMem::EmptyLatest() {
  pthread_mutex_lock(&mutex_dev_latest_);
  bool empty = dev_latest_.empty();
  pthread_mutex_unlock(&mutex_dev_latest_);
  return empty;
}

bool CLMem::HasLatest(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_latest_);
  set<CLDevice*>::iterator it = dev_latest_.find(device);
  bool find = (it != dev_latest_.end());
  pthread_mutex_unlock(&mutex_dev_latest_);
  return find;
}

CLDevice* CLMem::FrontLatest() {
  CLDevice* device = NULL;
  pthread_mutex_lock(&mutex_dev_latest_);
  if (!dev_latest_.empty())
    device = *(dev_latest_.begin());
  pthread_mutex_unlock(&mutex_dev_latest_);
  return device;
}

void CLMem::AddLatest(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_latest_);
  dev_latest_.insert(device);
  pthread_mutex_unlock(&mutex_dev_latest_);
}

void CLMem::SetLatest(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_latest_);
  dev_latest_.clear();
  dev_latest_.insert(device);
  pthread_mutex_unlock(&mutex_dev_latest_);
}

CLDevice* CLMem::GetNearestLatest(CLDevice* device) {
  CLDevice* nearest = NULL;
  int min_distance = 10; // INF
  pthread_mutex_lock(&mutex_dev_latest_);
  for (set<CLDevice*>::iterator it = dev_latest_.begin();
       it != dev_latest_.end();
       ++it) {
    int distance = device->GetDistance(*it);
    if (distance < min_distance) {
      nearest = *it;
      min_distance = distance;
    } 
  }
  pthread_mutex_unlock(&mutex_dev_latest_);
  return nearest;
}

void* CLMem::MapAsBuffer(cl_map_flags map_flags, size_t offset, size_t size) {
  Retain();

  void* ptr;
  if (use_host_)
    ptr = (void*)((size_t)host_ptr_ + offset);
  else
    ptr = malloc(size);

  CLMapWritebackLayout wb_layout;
  wb_layout.origin[0] = offset;
  wb_layout.region[0] = size;

  pthread_mutex_lock(&mutex_map_);
  map_count_++;
  if (!use_host_ &&
      map_flags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION)) {
    map_writeback_[ptr] = wb_layout;
  }
  pthread_mutex_unlock(&mutex_map_);
  return ptr;
}

void* CLMem::MapAsImage(cl_map_flags map_flags, const size_t* origin,
                        const size_t* region, size_t* image_row_pitch,
                        size_t* image_slice_pitch) {
  Retain();

  void* ptr;
  if (use_host_) {
    size_t offset;
    switch (type_) {
      case CL_MEM_OBJECT_IMAGE3D:
      case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        offset = origin[2] * image_slice_pitch_ +
                 origin[1] * image_row_pitch_ +
                 origin[0] * image_element_size_;
        break;
      case CL_MEM_OBJECT_IMAGE2D:
        offset = origin[1] * image_row_pitch_ +
                 origin[0] * image_element_size_;
        break;
      case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        offset = origin[1] * image_slice_pitch_ +
                 origin[0] * image_element_size_;
        break;
      case CL_MEM_OBJECT_IMAGE1D:
        offset = origin[0] * image_element_size_;
        break;
      default:
        offset = 0;
        break;
    }
    ptr = (void*)((size_t)host_ptr_ + offset);
    *image_row_pitch = image_row_pitch_;
    if (image_slice_pitch)
      *image_slice_pitch = image_slice_pitch_;
  } else {
    size_t size = region[2] * region[1] * region[0];
    ptr = malloc(size);
    *image_row_pitch = region[0] * image_element_size_;
    if (image_slice_pitch) {
      switch (type_) {
        case CL_MEM_OBJECT_IMAGE3D:
        case CL_MEM_OBJECT_IMAGE2D_ARRAY:
          *image_slice_pitch = region[1] * region[0] * image_element_size_;
          break;
        case CL_MEM_OBJECT_IMAGE1D_ARRAY:
          *image_slice_pitch = region[0] * image_element_size_;
          break;
        default:
          *image_slice_pitch = 0;
          break;
      }
    }
  }

  CLMapWritebackLayout wb_layout;
  memcpy(wb_layout.origin, origin, sizeof(size_t) * 3);
  memcpy(wb_layout.region, region, sizeof(size_t) * 3);

  pthread_mutex_lock(&mutex_map_);
  map_count_++;
  if (!use_host_ &&
      map_flags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION)) {
    map_writeback_[ptr] = wb_layout;
  }
  pthread_mutex_unlock(&mutex_map_);
  return ptr;
}

bool CLMem::IsMapInitRequired(void* ptr) {
  return !use_host_;
}

bool CLMem::GetMapWritebackLayoutForBuffer(void* ptr, size_t* offset,
                                           size_t* size) {
  bool required = false;
  pthread_mutex_lock(&mutex_map_);
  if (map_writeback_.count(ptr)) {
    required = true;
    *offset = map_writeback_[ptr].origin[0];
    *size = map_writeback_[ptr].region[0];
    map_writeback_.erase(ptr);
  }
  pthread_mutex_unlock(&mutex_map_);
  return required;
}

bool CLMem::GetMapWritebackLayoutForImage(void* ptr, size_t* origin,
                                          size_t* region) {
  bool required = false;
  pthread_mutex_lock(&mutex_map_);
  if (map_writeback_.count(ptr)) {
    required = true;
    memcpy(origin, map_writeback_[ptr].origin, sizeof(size_t) * 3);
    memcpy(region, map_writeback_[ptr].region, sizeof(size_t) * 3);
    map_writeback_.erase(ptr);
  }
  pthread_mutex_unlock(&mutex_map_);
  return required;
}

void CLMem::Unmap(void* ptr) {
  if (!use_host_)
    free(ptr);
  Release();
}

void CLMem::AddDestructorCallback(MemObjectDestructorCallback* callback) {
  pthread_mutex_lock(&mutex_callbacks_);
  callbacks_.push_back(callback);
  pthread_mutex_unlock(&mutex_callbacks_);
}

void CLMem::SetHostPtr(void* host_ptr) {
  if (flags_ & CL_MEM_USE_HOST_PTR) {
    host_ptr_ = host_ptr;
    use_host_ = true;
  } else if ((flags_ & CL_MEM_ALLOC_HOST_PTR) &&
             (flags_ & CL_MEM_COPY_HOST_PTR)) {
    host_ptr_ = memalign(4096, size_);
    memcpy(host_ptr_, host_ptr, size_);
    alloc_host_ = true;
    use_host_ = true;
  } else if (flags_ & CL_MEM_ALLOC_HOST_PTR) {
    host_ptr_ = memalign(4096, size_);
    alloc_host_ = true;
    use_host_ = true;
  } else if (flags_ & CL_MEM_COPY_HOST_PTR) {
    host_ptr_ = memalign(4096, size_);
    memcpy(host_ptr_, host_ptr, size_);
    alloc_host_ = true;
    SetLatest(LATEST_HOST);
  }
}

CLMem* CLMem::CreateBuffer(CLContext* context, cl_mem_flags flags, size_t size,
                           void* host_ptr, cl_int* err) {
  if (!(flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY)))
    flags |= flags & CL_MEM_READ_WRITE;

  CLMem* mem = new CLMem(context);
  if (mem == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  mem->type_ = CL_MEM_OBJECT_BUFFER;
  mem->flags_ = flags;
  mem->size_ = size;
  mem->offset_ = 0;
  mem->SetHostPtr(host_ptr);
  return mem;
}

CLMem* CLMem::CreateSubBuffer(CLMem* parent, cl_mem_flags flags, size_t origin,
                              size_t size, cl_int* err) {
  parent->Retain();
  parent->InheritFlags(flags);

  CLMem* mem = new CLMem(parent->context_);
  if (mem == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  mem->type_ = CL_MEM_OBJECT_BUFFER;
  mem->flags_ = flags;
  mem->size_ = size;
  mem->parent_ = parent;
  mem->offset_ = origin;
  mem->host_ptr_ = (parent->host_ptr_ == NULL ? NULL :
                    (void*)((size_t)parent->host_ptr_ + origin));
  mem->alloc_host_ = false;
  mem->use_host_ = parent->use_host_;

  return mem;
}

CLMem* CLMem::CreateImage(CLContext* context, cl_mem_flags flags,
                          const cl_image_format* image_format,
                          const cl_image_desc* image_desc, void* host_ptr,
                          cl_int* err) {
  if (!(flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY)))
    flags |= flags & CL_MEM_READ_WRITE;

  size_t width = image_desc->image_width;
  size_t height = image_desc->image_height;
  size_t depth = image_desc->image_depth;
  size_t array_size = image_desc->image_array_size;
  size_t row_pitch = image_desc->image_row_pitch;
  size_t slice_pitch = image_desc->image_slice_pitch;

  size_t element_size, channels;
  InterpretImageFormat(image_format, element_size, channels);

  if (host_ptr == NULL && (row_pitch != 0 || slice_pitch != 0)) {
    *err = CL_INVALID_IMAGE_DESCRIPTOR;
    return NULL;
  }

  if (row_pitch == 0) {
    row_pitch = width * element_size;
  } else if (row_pitch < width * element_size ||
             row_pitch % element_size != 0) {
    *err = CL_INVALID_IMAGE_DESCRIPTOR;
    return NULL;
  }
  switch (image_desc->image_type) {
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE3D:
      if (slice_pitch == 0) {
        slice_pitch = row_pitch * height;
      } else if (slice_pitch < row_pitch * height ||
                 slice_pitch % row_pitch == 0) {
        *err = CL_INVALID_IMAGE_DESCRIPTOR;
        return NULL;
      }
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      if (slice_pitch == 0) {
        slice_pitch = row_pitch;
      } else if (slice_pitch < row_pitch ||
                 slice_pitch % row_pitch == 0) {
        *err = CL_INVALID_IMAGE_DESCRIPTOR;
        return NULL;
      }
      break;
    default: break;
  }

  size_t size;
  size_t region[3];
  switch (image_desc->image_type) {
    case CL_MEM_OBJECT_IMAGE3D:
      region[0] = width;
      region[1] = height;
      region[2] = depth;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      region[0] = width;
      region[1] = height;
      region[2] = array_size;
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      region[0] = width;
      region[1] = height;
      region[2] = 1;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      region[0] = width;
      region[1] = array_size;
      region[2] = 1;
      break;
    case CL_MEM_OBJECT_IMAGE1D:
      region[0] = width;
      region[1] = 1;
      region[2] = 1;
      break;
  }
  size = region[0] * region[1] * region[2] * element_size;

  CLMem* mem = new CLMem(context);
  if (mem == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  mem->type_ = image_desc->image_type;
  mem->flags_ = flags;
  mem->size_ = size;
  mem->parent_ = NULL;
  mem->offset_ = 0;
  mem->SetHostPtr(host_ptr);
  mem->image_format_ = *image_format;
  mem->image_desc_ = *image_desc;
  mem->image_element_size_ = element_size;
  mem->image_channels_ = channels;
  mem->image_row_pitch_ = row_pitch;
  mem->image_slice_pitch_ = slice_pitch;
  memcpy(mem->image_region_, region, sizeof(size_t) * 3);
  return mem;
}

void CLMem::InterpretImageFormat(const cl_image_format* image_format,
                                 size_t& element_size, size_t& channels) {
  switch (image_format->image_channel_order) {
    case CL_R:
    case CL_A:
    case CL_INTENSITY:
    case CL_LUMINANCE:
    case CL_Rx:
      channels = 1;
      break;
    case CL_RG:
    case CL_RA:
    case CL_RGx:
      channels = 2;
      break;
    case CL_RGB:
    case CL_RGBx:
      channels = 3;
      break;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
      channels = 4;
      break;
    default:
      return;
  }

  switch (image_format->image_channel_data_type) {
    case CL_SNORM_INT8:
    case CL_UNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
      element_size = 1;
      break;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_UNORM_SHORT_565:
    case CL_UNORM_SHORT_555:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
      element_size = 2;
      break;
    case CL_UNORM_INT_101010:
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_FLOAT:
      element_size = 4;
      break;
    default:
      return;
  }
  element_size *= channels;
}
