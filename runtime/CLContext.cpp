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

#include "CLContext.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "Callbacks.h"
#include "CLDevice.h"
#include "CLDispatch.h"
#include "CLMem.h"
#include "CLObject.h"
#include "CLSampler.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLContext::CLContext(const std::vector<CLDevice*>& devices,
                     size_t num_properties,
                     const cl_context_properties* properties)
    : devices_(devices) {
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    (*it)->Retain();
  }

  num_properties_ = num_properties;
  if (num_properties > 0) {
    properties_ = (cl_context_properties*)malloc(
        sizeof(cl_context_properties) * num_properties);
    memcpy(properties_, properties,
           sizeof(cl_context_properties) * num_properties);
  } else {
    properties_ = NULL;
  }
  callback_ = NULL;

  pthread_mutex_init(&mutex_mems_, NULL);
  pthread_mutex_init(&mutex_samplers_, NULL);

  InitImageInfo();
}

CLContext::~CLContext() {
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    (*it)->Release();
  }
  if (properties_ != NULL)
    free(properties_);
  if (callback_ != NULL)
    delete callback_;

  pthread_mutex_destroy(&mutex_mems_);
  pthread_mutex_destroy(&mutex_samplers_);
}

cl_int CLContext::GetContextInfo(cl_context_info param_name,
                                 size_t param_value_size, void* param_value,
                                 size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_CONTEXT_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO_T(CL_CONTEXT_NUM_DEVICES, cl_uint, devices_.size());
    case CL_CONTEXT_DEVICES: {
      size_t size = sizeof(cl_device_id) * devices_.size();
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        for (size_t i = 0; i < devices_.size(); i++)
          ((cl_device_id*)param_value)[i] = devices_[i]->st_obj();
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    GET_OBJECT_INFO_A(CL_CONTEXT_PROPERTIES, cl_context_properties,
                      properties_, num_properties_);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLContext::GetSupportedImageFormats(cl_mem_flags flags,
                                           cl_mem_object_type image_type,
                                           cl_uint num_entries,
                                           cl_image_format* image_formats,
                                           cl_uint* num_image_formats) {
  // Currently we ignore flags and image_type
  if (image_formats) {
    for (size_t i = 0; i < supported_image_formats_.size(); i++) {
      if (i < num_entries)
        image_formats[i] = supported_image_formats_[i];
    }
  }
  if (num_image_formats) *num_image_formats = supported_image_formats_.size();
  return CL_SUCCESS;
}

bool CLContext::IsValidDevice(CLDevice* device) {
  return (find(devices_.begin(), devices_.end(), device) != devices_.end());
}

bool CLContext::IsValidDevices(cl_uint num_devices,
                               const cl_device_id* device_list) {
  for (cl_uint i = 0; i < num_devices; i++) {
    if (device_list[i] == NULL || !IsValidDevice(device_list[i]->c_obj))
      return false;
  }
  return true;
}

bool CLContext::IsValidMem(cl_mem mem) {
  bool valid = false;
  pthread_mutex_lock(&mutex_mems_);
  for (vector<CLMem*>::iterator it = mems_.begin();
       it != mems_.end();
       ++it) {
    if ((*it)->st_obj() == mem) {
      valid = true;
      break;
    }
  }
  pthread_mutex_unlock(&mutex_mems_);
  return valid;
}

bool CLContext::IsValidSampler(cl_sampler sampler) {
  bool valid = false;
  pthread_mutex_lock(&mutex_samplers_);
  for (vector<CLSampler*>::iterator it = samplers_.begin();
       it != samplers_.end();
       ++it) {
    if ((*it)->st_obj() == sampler) {
      valid = true;
      break;
    }
  }
  pthread_mutex_unlock(&mutex_samplers_);
  return valid;
}

bool CLContext::IsSupportedImageFormat(cl_mem_flags flags,
                                       cl_mem_object_type image_type,
                                       const cl_image_format* image_format) {
  // Currently we ignore flags and image_type
  for (vector<cl_image_format>::iterator it = supported_image_formats_.begin();
       it != supported_image_formats_.end();
       ++it) {
    if ((*it).image_channel_order == image_format->image_channel_order &&
        (*it).image_channel_data_type == image_format->image_channel_data_type)
      return true;
  }
  return false;
}

bool CLContext::IsSupportedImageSize(const cl_image_desc* image_desc) {
  cl_mem_object_type type = image_desc->image_type;
  size_t width = image_desc->image_width;
  switch (type) {
    case CL_MEM_OBJECT_IMAGE2D:
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      if (width > supported_image2d_max_width_) return false;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      if (width > supported_image3d_max_width_) return false;
      break;
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      if (width > supported_image_max_buffer_size_) return false;
      break;
    default: break;
  }
  size_t height = image_desc->image_height;
  switch (type) {
    case CL_MEM_OBJECT_IMAGE2D:
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      if (height > supported_image2d_max_height_) return false;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      if (height > supported_image3d_max_height_) return false;
      break;
    default: break;
  }
  size_t depth = image_desc->image_depth;
  if (type == CL_MEM_OBJECT_IMAGE3D) {
    if (depth > supported_image3d_max_depth_) return false;
  }
  size_t array_size = image_desc->image_array_size;
  if (type == CL_MEM_OBJECT_IMAGE1D_ARRAY ||
      type == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
    if (array_size > supported_image_max_array_size_) return false;
  }
  return true;
}

void CLContext::AddMem(CLMem* mem) {
  pthread_mutex_lock(&mutex_mems_);
  mems_.push_back(mem);
  pthread_mutex_unlock(&mutex_mems_);
}

void CLContext::RemoveMem(CLMem* mem) {
  pthread_mutex_lock(&mutex_mems_);
  vector<CLMem*>::iterator it = find(mems_.begin(), mems_.end(), mem);
  if (it != mems_.end())
    mems_.erase(it);
  pthread_mutex_unlock(&mutex_mems_);
}

void CLContext::AddSampler(CLSampler* sampler) {
  pthread_mutex_lock(&mutex_samplers_);
  samplers_.push_back(sampler);
  pthread_mutex_unlock(&mutex_samplers_);
}

void CLContext::RemoveSampler(CLSampler* sampler) {
  pthread_mutex_lock(&mutex_samplers_);
  vector<CLSampler*>::iterator it = find(samplers_.begin(), samplers_.end(),
                                         sampler);
  if (it != samplers_.end())
    samplers_.erase(it);
  pthread_mutex_unlock(&mutex_samplers_);
}

void CLContext::SetErrorNotificationCallback(
    ContextErrorNotificationCallback* callback) {
  if (callback_ != NULL)
    delete callback_;
  callback_ = callback;
}

void CLContext::NotifyError(const char* errinfo, const void* private_info,
                            size_t cb) {
  if (callback_ != NULL)
    callback_->run(errinfo, private_info, cb);
}

void CLContext::InitImageInfo() {
  image_supported_ = false;
  supported_image2d_max_width_ = -1;
  supported_image2d_max_height_ = -1;
  supported_image3d_max_width_ = -1;
  supported_image3d_max_height_ = -1;
  supported_image3d_max_depth_ = -1;
  supported_image_max_buffer_size_ = -1;
  supported_image_max_array_size_ = -1;
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    CLDevice* device = *it;
    if (device->IsImageSupported())
      image_supported_ = true;
    device->JoinSupportedImageSize(supported_image2d_max_width_,
                                   supported_image2d_max_height_,
                                   supported_image3d_max_width_,
                                   supported_image3d_max_height_,
                                   supported_image3d_max_depth_,
                                   supported_image_max_buffer_size_,
                                   supported_image_max_array_size_);
  }
  /*
   * Currently SnuCL only supports the minimum set of image formats.
   * See Section 5.3.2.1 of the OpenCL 1.2 specification
   */
  const cl_channel_order orders[] = {CL_RGBA, CL_BGRA};
  const cl_channel_type data_types[] = {
      CL_UNORM_INT8, CL_UNORM_INT16, CL_SIGNED_INT8, CL_SIGNED_INT16,
      CL_SIGNED_INT32, CL_UNSIGNED_INT8, CL_UNSIGNED_INT16, CL_UNSIGNED_INT32,
      CL_HALF_FLOAT, CL_FLOAT};
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 10; j++) {
      cl_image_format format = {orders[i], data_types[j]};
      supported_image_formats_.push_back(format);
    }
}
