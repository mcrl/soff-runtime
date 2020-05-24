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

#include "CLKernel.h"
#include <cstdlib>
#include <cstring>
#include <map>
#include <pthread.h>
#include <CL/cl.h>
#include "CLContext.h"
#include "CLDevice.h"
#include "CLMem.h"
#include "CLObject.h"
#include "CLProgram.h"
#include "CLSampler.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLKernel::CLKernel(CLContext* context, CLProgram* program,
                   CLKernelInfo* kernel_info) {
  context_ = context;
  context_->Retain();
  program_ = program;
  program_->Retain();
  kernel_info_ = kernel_info;

  name_ = kernel_info->name();
  args_dirty_ = false;

  pthread_mutex_init(&mutex_dev_specific_, NULL);
}

void CLKernel::Cleanup() {
  for (map<CLDevice*, void*>::iterator it = dev_specific_.begin();
       it != dev_specific_.end();
       ++it) {
    (it->first)->FreeKernel(this, it->second);
  }
}

CLKernel::~CLKernel() {
  for (map<cl_uint, CLKernelArg*>::iterator it = args_.begin();
       it != args_.end();
       ++it) {
    free(it->second);
  }
  program_->ReleaseKernel();
  program_->Release();
  context_->Release();

  pthread_mutex_destroy(&mutex_dev_specific_);
}

cl_uint CLKernel::num_args() const {
  return kernel_info_->num_args();
}

int CLKernel::snucl_index() const {
  return kernel_info_->snucl_index();
}

cl_int CLKernel::GetKernelInfo(cl_kernel_info param_name,
                               size_t param_value_size, void* param_value,
                               size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_S(CL_KERNEL_FUNCTION_NAME, name_);
    GET_OBJECT_INFO_T(CL_KERNEL_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO_T(CL_KERNEL_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO_T(CL_KERNEL_PROGRAM, cl_program, program_->st_obj());
    GET_OBJECT_INFO_T(CL_KERNEL_NUM_ARGS, cl_uint, kernel_info_->num_args());
    GET_OBJECT_INFO_S(CL_KERNEL_ATTRIBUTES, kernel_info_->attributes());
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLKernel::GetKernelWorkGroupInfo(CLDevice* device,
                                        cl_kernel_work_group_info param_name,
                                        size_t param_value_size,
                                        void* param_value,
                                        size_t* param_value_size_ret) {
  return kernel_info_->GetKernelWorkGroupInfo(device, param_name,
                                              param_value_size, param_value,
                                              param_value_size_ret);
}

cl_int CLKernel::GetKernelArgInfo(cl_uint arg_index,
                                  cl_kernel_arg_info param_name,
                                  size_t param_value_size, void* param_value,
                                  size_t* param_value_size_ret) {
  return kernel_info_->GetKernelArgInfo(arg_index, param_name,
                                        param_value_size, param_value,
                                        param_value_size_ret);
}

cl_int CLKernel::SetKernelArg(cl_uint arg_index, size_t arg_size,
                              const void* arg_value) {
  if (arg_size > 256 && arg_value != NULL)
    return CL_INVALID_ARG_SIZE;

  CLKernelArg* arg;
  map<cl_uint, CLKernelArg*>::iterator old = args_.find(arg_index);
  if (old != args_.end()) {
    arg = old->second;
  } else {
    arg = (CLKernelArg*)malloc(sizeof(CLKernelArg));
    if (arg == NULL)
      return CL_OUT_OF_HOST_MEMORY;
    args_[arg_index] = arg;
  }

  arg->size = arg_size;
  if (arg_value != NULL) {
    memcpy(arg->value, arg_value, arg_size);
    arg->local = false;
    cl_mem m = *((cl_mem*)arg_value);
    arg->mem = context_->IsValidMem(m) ? m->c_obj : NULL;
    cl_sampler s = *((cl_sampler*)arg_value);
    arg->sampler = context_->IsValidSampler(s) ? s->c_obj : NULL;
  } else {
    arg->local = true;
    arg->mem = NULL;
    arg->sampler = NULL;
  }

  args_dirty_ = true;

  return CL_SUCCESS;
}

map<cl_uint, CLKernelArg*>* CLKernel::ExportArgs() {
  map<cl_uint, CLKernelArg*>* new_args = new map<cl_uint, CLKernelArg*>();
  for (map<cl_uint, CLKernelArg*>::iterator it = args_.begin();
       it != args_.end();
       ++it) {
    CLKernelArg* arg = it->second;
    CLKernelArg* new_arg = (CLKernelArg*)malloc(sizeof(CLKernelArg));
    new_arg->size = arg->size;
    if (!arg->local)
      memcpy(new_arg->value, arg->value, arg->size);
    new_arg->local = arg->local;
    new_arg->mem = arg->mem;
    if (new_arg->mem != NULL)
      new_arg->mem->Retain();
    new_arg->sampler = arg->sampler;
    if (new_arg->sampler != NULL)
      new_arg->sampler->Retain();

    (*new_args)[it->first] = new_arg;
  }
  return new_args;
}

bool CLKernel::HasDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  bool alloc = (dev_specific_.count(device) > 0);
  pthread_mutex_unlock(&mutex_dev_specific_);
  return alloc;
}

void* CLKernel::GetDevSpecific(CLDevice* device) {
  pthread_mutex_lock(&mutex_dev_specific_);
  void* dev_specific;
  if (dev_specific_.count(device) > 0) {
    dev_specific = dev_specific_[device];
  } else {
    pthread_mutex_unlock(&mutex_dev_specific_);
    dev_specific = device->AllocKernel(this);
    pthread_mutex_lock(&mutex_dev_specific_);
    dev_specific_[device] = dev_specific;
  }
  pthread_mutex_unlock(&mutex_dev_specific_);
  return dev_specific;
}
