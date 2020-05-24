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

#ifndef __SNUCL__CL_CONTEXT_H
#define __SNUCL__CL_CONTEXT_H

#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

class CLDevice;
class ContextErrorNotificationCallback;

class CLContext: public CLObject<struct _cl_context, CLContext> {
 public:
  CLContext(const std::vector<CLDevice*>& devices, size_t num_properties,
            const cl_context_properties* properties);
  virtual ~CLContext();

  const std::vector<CLDevice*>& devices() const { return devices_; }

  cl_int GetContextInfo(cl_context_info param_name, size_t param_value_size,
                        void* param_value, size_t* param_value_size_ret);
  cl_int GetSupportedImageFormats(cl_mem_flags flags,
                                  cl_mem_object_type image_type,
                                  cl_uint num_entries,
                                  cl_image_format* image_formats,
                                  cl_uint* num_image_formats);

  bool IsValidDevice(CLDevice* device);
  bool IsValidDevices(cl_uint num_devices, const cl_device_id* device_list);
  bool IsValidMem(cl_mem mem);
  bool IsValidSampler(cl_sampler sampler);

  bool IsImageSupported() const { return image_supported_; }
  bool IsSupportedImageFormat(cl_mem_flags flags,
                              cl_mem_object_type image_type,
                              const cl_image_format* image_format);
  bool IsSupportedImageSize(const cl_image_desc* image_desc);

  void AddMem(CLMem* mem);
  void RemoveMem(CLMem* mem);
  void AddSampler(CLSampler* sampler);
  void RemoveSampler(CLSampler* sampler);

  void SetErrorNotificationCallback(
      ContextErrorNotificationCallback* callback);
  void NotifyError(const char* errinfo, const void* private_info, size_t cb);

 private:
  void InitImageInfo();

  std::vector<CLDevice*> devices_;
  size_t num_properties_;
  cl_context_properties* properties_;
  std::vector<CLMem*> mems_;
  std::vector<CLSampler*> samplers_;

  bool image_supported_;
  std::vector<cl_image_format> supported_image_formats_;
  size_t supported_image2d_max_width_;
  size_t supported_image2d_max_height_;
  size_t supported_image3d_max_width_;
  size_t supported_image3d_max_height_;
  size_t supported_image3d_max_depth_;
  size_t supported_image_max_buffer_size_;
  size_t supported_image_max_array_size_;

  ContextErrorNotificationCallback* callback_;

  pthread_mutex_t mutex_mems_;
  pthread_mutex_t mutex_samplers_;
};

#endif // __SNUCL__CL_CONTEXT_H
