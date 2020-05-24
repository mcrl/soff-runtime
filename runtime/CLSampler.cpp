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

#include "CLSampler.h"
#include <cstring>
#include <map>
#include <pthread.h>
#include <CL/cl.h>
#include "CLContext.h"
#include "CLDevice.h"
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLSampler::CLSampler(CLContext *context, cl_bool normalized_coords,
                     cl_addressing_mode addressing_mode,
                     cl_filter_mode filter_mode) {
  context_ = context;
  context_->Retain();
  context_->AddSampler(this);

  normalized_coords_ = normalized_coords;
  addressing_mode_ = addressing_mode;
  filter_mode_ = filter_mode;

  pthread_mutex_init(&mutex_dev_specific_, NULL);
}

void CLSampler::Cleanup() {
  for (map<CLDevice*, void*>::iterator it = dev_specific_.begin();
       it != dev_specific_.end();
       ++it) {
    (it->first)->FreeSampler(this, it->second);
  }
  context_->RemoveSampler(this);
}

CLSampler::~CLSampler() {
  context_->Release();

  pthread_mutex_destroy(&mutex_dev_specific_);
}

cl_int CLSampler::GetSamplerInfo(cl_sampler_info param_name,
                                 size_t param_value_size, void* param_value,
                                 size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_SAMPLER_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO_T(CL_SAMPLER_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO(CL_SAMPLER_NORMALIZED_COORDS, cl_bool, normalized_coords_);
    GET_OBJECT_INFO(CL_SAMPLER_ADDRESSING_MODE, cl_addressing_mode,
                    addressing_mode_);
    GET_OBJECT_INFO(CL_SAMPLER_FILTER_MODE, cl_filter_mode, filter_mode_);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

bool CLSampler::HasDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  bool alloc = (dev_specific_.count(device) > 0);
  pthread_mutex_unlock(&mutex_dev_specific_);
  return alloc;
}

void* CLSampler::GetDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  void* dev_specific;
  if (dev_specific_.count(device) > 0) {
    dev_specific = dev_specific_[device];
  } else {
    pthread_mutex_unlock(&mutex_dev_specific_);
    dev_specific = device->AllocSampler(this);
    pthread_mutex_lock(&mutex_dev_specific_);
    dev_specific_[device] = dev_specific;
  }
  pthread_mutex_unlock(&mutex_dev_specific_);
  return dev_specific;
}
