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

#include "CLProgram.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <pthread.h>
#include <stdint.h>
#include <CL/cl.h>
#include "Callbacks.h"
#include "CLContext.h"
#include "CLDevice.h"
#include "CLEvent.h"
#include "CLKernel.h"
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLProgramSource::CLProgramSource(CLProgramSource* original)
    : sources_(original->sources_), concat_source_(original->concat_source_),
      header_name_(original->header_name_) {
}

void CLProgramSource::AddSource(const char* source) {
  sources_.push_back(string(source));
  concat_source_.append(source);
}

void CLProgramSource::AddSource(const char* source, size_t length) {
  sources_.push_back(string(source, length));
  concat_source_.append(source, length);
}

void CLProgramSource::SetHeaderName(const char* header_name) {
  header_name_ = std::string(header_name);
}

CLProgramBinary::CLProgramBinary(CLDevice* device,
                                 const unsigned char* full_binary,
                                 size_t size) {
  device_ = device;
  type_ = ParseHeader(full_binary, size);
  if (type_ == CL_PROGRAM_BINARY_TYPE_NONE) {
    binary_ = NULL;
    size_ = 0;
  } else {
    binary_ = (unsigned char*)malloc(sizeof(unsigned char) * size);
    memcpy(binary_, full_binary, sizeof(unsigned char) * size);
    size_ = size;
  }
}

CLProgramBinary::CLProgramBinary(CLDevice* device, cl_program_binary_type type,
                                 const unsigned char* core_binary,
                                 size_t size) {
  device_ = device;
  type_ = type;
  binary_ = (unsigned char*)malloc(sizeof(unsigned char) * (size + 16));
  GenerateHeader(binary_, type, core_binary, size);
  memcpy(binary_ + 16, core_binary, sizeof(unsigned char) * size);
  size_ = size + 16;
}

CLProgramBinary::CLProgramBinary(CLProgramBinary* original) {
  device_ = original->device_;
  type_ = original->type_;
  size_ = original->size_;
  binary_ = (unsigned char*)malloc(sizeof(unsigned char) * size_);
  memcpy(binary_, original->binary_, size_);
}

CLProgramBinary::~CLProgramBinary() {
  if (binary_)
    free(binary_);
}

unsigned int CLProgramBinary::Hash(const unsigned char* str, size_t size) {
  // RS hash function
  unsigned int a = 63689;
  unsigned int b = 378551;
  unsigned int hash = 0;
  while (size > 0) {
    hash = hash * a + *str;
    a = a * b;
    str++;
    size--;
  }
  return (hash & 0x7FFFFFFF);
}

cl_program_binary_type CLProgramBinary::ParseHeader(
    const unsigned char* full_binary, size_t size) {
  if (size < 16) return CL_PROGRAM_BINARY_TYPE_NONE;
  if (full_binary[0] != 'B' || full_binary[1] != 'i' ||
      full_binary[2] != 'n')
    return CL_PROGRAM_BINARY_TYPE_NONE;

  unsigned int hash;
  memcpy(&hash, full_binary + 4, sizeof(unsigned int));
  if (hash != Hash(full_binary + 16, size - 16))
    return CL_PROGRAM_BINARY_TYPE_NONE;
  uint64_t size_in_header;
  memcpy(&size_in_header, full_binary + 8, sizeof(uint64_t));
  if ((size_t)size_in_header != size - 16)
    return CL_PROGRAM_BINARY_TYPE_NONE;

  switch (full_binary[3]) {
    case 'C': return CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
    case 'L': return CL_PROGRAM_BINARY_TYPE_LIBRARY;
    case 'E': return CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
    default: return CL_PROGRAM_BINARY_TYPE_NONE;
  }
}

void CLProgramBinary::GenerateHeader(unsigned char* header,
                                     cl_program_binary_type type,
                                     const unsigned char* core_binary,
                                     size_t size) {
  header[0] = 'B';
  header[1] = 'i';
  header[2] = 'n';
  switch (type) {
    case CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT:
      header[3] = 'C';
      break;
    case CL_PROGRAM_BINARY_TYPE_LIBRARY:
      header[3] = 'L';
      break;
    case CL_PROGRAM_BINARY_TYPE_EXECUTABLE:
      header[3] = 'E';
      break;
    default:
      header[3] = 'X';
      break;
  }

  unsigned int hash = Hash(core_binary, size);
  memcpy(header + 4, &hash, sizeof(unsigned int));
  uint64_t size_in_header = (uint64_t)size;
  memcpy(header + 8, &size_in_header, sizeof(uint64_t));
}

CLKernelInfo::CLKernelInfo(const char* name, cl_uint num_args,
                           const char* attributes) {
  name_ = (char*)malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(name_, name);
  num_args_ = num_args;
  attributes_ = (char*)malloc(sizeof(char) * (strlen(attributes) + 1));
  strcpy(attributes_, attributes);
  valid_ = true;

  arg_address_qualifiers_ =
    (cl_kernel_arg_address_qualifier*)malloc(
      sizeof(cl_kernel_arg_address_qualifier) * num_args_);
  arg_access_qualifiers_ =
    (cl_kernel_arg_access_qualifier*)malloc(
      sizeof(cl_kernel_arg_access_qualifier) * num_args_);
  arg_type_names_ = (char**)malloc(sizeof(char*) * num_args_);
  for (cl_uint i = 0; i < num_args_; i++)
    arg_type_names_[i] = NULL;
  arg_type_qualifiers_ =
    (cl_kernel_arg_type_qualifier*)malloc(
      sizeof(cl_kernel_arg_type_qualifier) * num_args_);
  arg_names_ = (char**)malloc(sizeof(char*) * num_args_);
  for (cl_uint i = 0; i < num_args_; i++)
    arg_names_[i] = NULL;

  snucl_index_ = -1;
}

CLKernelInfo::~CLKernelInfo() {
  free(name_);
  free(attributes_);
  free(arg_address_qualifiers_);
  free(arg_access_qualifiers_);
  for (cl_uint i = 0; i < num_args_; i++)
    if (arg_type_names_[i] != NULL)
      free(arg_type_names_[i]);
  free(arg_type_names_);
  free(arg_type_qualifiers_);
  for (cl_uint i = 0; i < num_args_; i++)
    if (arg_names_[i] != NULL)
      free(arg_names_[i]);
  free(arg_names_);
}

cl_int CLKernelInfo::GetKernelWorkGroupInfo(
    CLDevice* device, cl_kernel_work_group_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret) {
  if (device == NULL) {
    if (devices_.size() > 1)
      return CL_INVALID_DEVICE;
    device = *devices_.begin();
  } else {
    if (devices_.count(device) == 0)
      return CL_INVALID_DEVICE;
  }
  switch (param_name) {
    case CL_KERNEL_GLOBAL_WORK_SIZE:
      return CL_INVALID_VALUE;
    GET_OBJECT_INFO_T(CL_KERNEL_WORK_GROUP_SIZE, size_t,
                      work_group_size_[device]);
    GET_OBJECT_INFO_A(CL_KERNEL_COMPILE_WORK_GROUP_SIZE, size_t,
                      compile_work_group_size_, 3);
    GET_OBJECT_INFO_T(CL_KERNEL_LOCAL_MEM_SIZE, cl_ulong,
                      local_mem_size_[device]);
    GET_OBJECT_INFO_T(CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, size_t,
                      preferred_work_group_size_multiple_[device]);
    GET_OBJECT_INFO_T(CL_KERNEL_PRIVATE_MEM_SIZE, cl_ulong,
                      private_mem_size_[device]);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLKernelInfo::GetKernelArgInfo(cl_uint arg_index,
                                      cl_kernel_arg_info param_name,
                                      size_t param_value_size,
                                      void* param_value,
                                      size_t* param_value_size_ret) {
  if (indexes_.count(arg_index) == 0)
    return CL_KERNEL_ARG_INFO_NOT_AVAILABLE;
  switch (param_name) {
    GET_OBJECT_INFO(CL_KERNEL_ARG_ADDRESS_QUALIFIER,
                    cl_kernel_arg_address_qualifier,
                    arg_address_qualifiers_[arg_index]);
    GET_OBJECT_INFO(CL_KERNEL_ARG_ACCESS_QUALIFIER,
                    cl_kernel_arg_access_qualifier,
                    arg_access_qualifiers_[arg_index]);
    GET_OBJECT_INFO_S(CL_KERNEL_ARG_TYPE_NAME, arg_type_names_[arg_index]);
    GET_OBJECT_INFO(CL_KERNEL_ARG_TYPE_QUALIFIER, cl_kernel_arg_type_qualifier,
                    arg_type_qualifiers_[arg_index]);
    GET_OBJECT_INFO_S(CL_KERNEL_ARG_NAME, arg_names_[arg_index]);
  }
  return CL_SUCCESS;
}

void CLKernelInfo::Update(const char* name, cl_uint num_args,
                          const char* attributes) {
  if (strcmp(name_, name) != 0 || num_args_ != num_args ||
      strcmp(attributes_, attributes) != 0)
    valid_ = false;
}

void CLKernelInfo::SetWorkGroupInfo(CLDevice* device, size_t work_group_size,
                                    size_t compile_work_group_size[3],
                                    cl_ulong local_mem_size,
                                    size_t preferred_work_group_size_multiple,
                                    cl_ulong private_mem_size) {
  if (devices_.count(device) > 0) {
/*
 * Currently we don't invalidate the kernel info because OpenCL compilers of
 * different vendors may return different results.
 */
#if 0
    if (work_group_size_[device] != work_group_size ||
        compile_work_group_size_[0] != compile_work_group_size[0] ||
        compile_work_group_size_[1] != compile_work_group_size[1] ||
        compile_work_group_size_[2] != compile_work_group_size[2] ||
        local_mem_size_[device] != local_mem_size ||
        preferred_work_group_size_multiple_[device] !=
            preferred_work_group_size_multiple ||
        private_mem_size_[device] != private_mem_size) {
      valid_ = false;
    }
#endif
    return;
  }
  devices_.insert(device);
  work_group_size_[device] = work_group_size;
  memcpy(compile_work_group_size_, compile_work_group_size,
         sizeof(size_t) * 3);
  local_mem_size_[device] = local_mem_size;
  preferred_work_group_size_multiple_[device] =
      preferred_work_group_size_multiple;
  private_mem_size_[device] = private_mem_size;
}

void CLKernelInfo::SetArgInfo(
    cl_uint arg_index, cl_kernel_arg_address_qualifier arg_address_qualifier,
    cl_kernel_arg_access_qualifier arg_access_qualifier,
    const char* arg_type_name, cl_kernel_arg_type_qualifier arg_type_qualifier,
    const char* arg_name) {
  if (indexes_.count(arg_index) > 0) {
/*
 * Currently we don't invalidate the kernel info because OpenCL compilers of
 * different vendors may return different results.
 */
#if 0
    if (arg_address_qualifiers_[arg_index] != arg_address_qualifier ||
        arg_access_qualifiers_[arg_index] != arg_access_qualifier ||
        strcmp(arg_type_names_[arg_index], arg_type_name) != 0 ||
        arg_type_qualifiers_[arg_index] != arg_type_qualifier ||
        strcmp(arg_names_[arg_index], arg_name) != 0) {
      valid_ = false;
    }
#endif
    return;
  }
  indexes_.insert(arg_index);
  arg_address_qualifiers_[arg_index] = arg_address_qualifier;
  arg_access_qualifiers_[arg_index] = arg_access_qualifier;
  arg_type_names_[arg_index] =
      (char*)malloc(sizeof(char) * (strlen(arg_type_name) + 1));
  strcpy(arg_type_names_[arg_index], arg_type_name);
  arg_type_qualifiers_[arg_index] = arg_type_qualifier;
  arg_names_[arg_index] =
      (char*)malloc(sizeof(char) * (strlen(arg_name) + 1));
  strcpy(arg_names_[arg_index], arg_name);
}

void CLKernelInfo::SetSnuCLIndex(int snucl_index) {
  snucl_index_ = snucl_index;
}

void CLKernelInfo::Invalidate() {
  valid_ = false;
}

size_t CLKernelInfo::GetSerializationSize(CLDevice* device) {
  size_t size = 0;

#define SERIALIZE_INFO(type, value) size += sizeof(type);
#define SERIALIZE_INFO_T(type, value) size += sizeof(type);
#define SERIALIZE_INFO_A(type, value, length) size += sizeof(type) * length;
#define SERIALIZE_INFO_S(value) size += strlen(value) + 1;

  SERIALIZE_INFO_S(name_);
  SERIALIZE_INFO(cl_uint, num_args_);
  SERIALIZE_INFO_S(attributes_);
  SERIALIZE_INFO_T(size_t, work_group_size_[device]);
  SERIALIZE_INFO_A(size_t, compile_work_group_size_[device], 3);
  SERIALIZE_INFO_T(cl_ulong, local_mem_size_[device]);
  SERIALIZE_INFO_T(size_t, preferred_work_group_size_multiple_[device]);
  SERIALIZE_INFO_T(cl_ulong, private_mem_size_[device]);
  for (cl_uint i = 0; i < num_args_; i++) {
    if (indexes_.count(i) > 0) {
      SERIALIZE_INFO(cl_uint, i);
      SERIALIZE_INFO(cl_kernel_arg_address_qualifier,
                     arg_address_qualifiers_[i]);
      SERIALIZE_INFO(cl_kernel_arg_access_qualifier,
                     arg_access_qualifiers_[i]);
      SERIALIZE_INFO_S(arg_type_names_[i]);
      SERIALIZE_INFO(cl_kernel_arg_type_qualifier, arg_type_qualifiers_[i]);
      SERIALIZE_INFO_S(arg_names_[i]);
    }
  }
  SERIALIZE_INFO_T(cl_uint, num_args_);
  SERIALIZE_INFO(bool, valid_);

#undef SERIALIZE_INFO
#undef SERIALIZE_INFO_T
#undef SERIALIZE_INFO_A
#undef SERIALIZE_INFO_S

  return size;
}

void* CLKernelInfo::SerializeKernelInfo(void* buffer, CLDevice* device) {
  char* p = (char*)buffer;

#define SERIALIZE_INFO(type, value) \
  memcpy(p, &value, sizeof(type));  \
  p += sizeof(type);
#define SERIALIZE_INFO_T(type, value) \
  {                                   \
    type temp = value;                \
    memcpy(p, &temp, sizeof(type));   \
    p += sizeof(type);                \
  }
#define SERIALIZE_INFO_A(type, value, length) \
  memcpy(p, value, sizeof(type) * length);    \
  p += sizeof(type) * length;
#define SERIALIZE_INFO_S(value)    \
  {                                \
    size_t length = strlen(value); \
    strcpy(p, value);              \
    p += length + 1;               \
  }

  SERIALIZE_INFO_S(name_);
  SERIALIZE_INFO(cl_uint, num_args_);
  SERIALIZE_INFO_S(attributes_);
  SERIALIZE_INFO_T(size_t, work_group_size_[device]);
  SERIALIZE_INFO_A(size_t, compile_work_group_size_, 3);
  SERIALIZE_INFO_T(cl_ulong, local_mem_size_[device]);
  SERIALIZE_INFO_T(size_t, preferred_work_group_size_multiple_[device]);
  SERIALIZE_INFO_T(cl_ulong, private_mem_size_[device]);
  for (cl_uint i = 0; i < num_args_; i++) {
    if (indexes_.count(i) > 0) {
      SERIALIZE_INFO(cl_uint, i);
      SERIALIZE_INFO(cl_kernel_arg_address_qualifier,
                     arg_address_qualifiers_[i]);
      SERIALIZE_INFO(cl_kernel_arg_access_qualifier,
                     arg_access_qualifiers_[i]);
      SERIALIZE_INFO_S(arg_type_names_[i]);
      SERIALIZE_INFO(cl_kernel_arg_type_qualifier, arg_type_qualifiers_[i]);
      SERIALIZE_INFO_S(arg_names_[i]);
    }
  }
  SERIALIZE_INFO_T(cl_uint, num_args_);
  SERIALIZE_INFO(bool, valid_);

#undef SERIALIZE_INFO
#undef SERIALIZE_INFO_T
#undef SERIALIZE_INFO_A
#undef SERIALIZE_INFO_S

  return p;
}

CLProgram::CLProgram(CLContext* context, const vector<CLDevice*>& devices)
    : devices_(devices) {
  context_ = context;
  context_->Retain();

  source_ = NULL;
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    binary_[*it] = NULL;
    executable_[*it] = NULL;
    build_status_[*it] = CL_BUILD_NONE;
    build_options_[*it] = NULL;
    build_log_[*it] = NULL;
    build_callback_[*it] = NULL;
  }
  build_in_progress_cnt_ = 0;
  build_success_cnt_ = 0;
  kernels_ref_cnt_ = 0;

  pthread_mutex_init(&mutex_build_, NULL);
}

void CLProgram::Cleanup() {
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    if (executable_[*it] != NULL)
      (*it)->FreeExecutable(this, executable_[*it]);
  }
}

CLProgram::~CLProgram() {
  if (source_ != NULL)
    delete source_;
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    if (binary_[*it] != NULL)
      delete binary_[*it];
    if (build_options_[*it] != NULL)
      free(build_options_[*it]);
    if (build_log_[*it] != NULL)
      free(build_log_[*it]);
    if (build_callback_[*it] != NULL)
      delete build_callback_[*it];
  }
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    delete *it;
  }

  context_->Release();

  pthread_mutex_destroy(&mutex_build_);
}

cl_int CLProgram::GetProgramInfo(cl_program_info param_name,
                                 size_t param_value_size, void* param_value,
                                 size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_PROGRAM_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO_T(CL_PROGRAM_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO_T(CL_PROGRAM_NUM_DEVICES, cl_uint, devices_.size());
    case CL_PROGRAM_DEVICES: {
      size_t size = sizeof(cl_device_id) * devices_.size();
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        for (size_t i = 0; i < devices_.size(); i++)
          ((cl_device_id*)param_value)[i] = devices_[i]->st_obj();
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    GET_OBJECT_INFO_S(CL_PROGRAM_SOURCE,
                      (source_ == NULL ? NULL : source_->concat_source()));
    case CL_PROGRAM_BINARY_SIZES: {
      size_t size = sizeof(size_t) * devices_.size();
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        for (size_t i = 0; i < devices_.size(); i++) {
          CLDevice* device = devices_[i];
          if (binary_[device] != NULL && binary_[device]->IsValid())
            ((size_t*)param_value)[i] = binary_[device]->full_size();
          else
            ((size_t*)param_value)[i] = 0;
        }
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    case CL_PROGRAM_BINARIES: {
      size_t size = sizeof(unsigned char*) * devices_.size();
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        for (size_t i = 0; i < devices_.size(); i++) {
          CLDevice* device = devices_[i];
          if (((char**)param_value)[i] && binary_[device] != NULL &&
              binary_[device]->IsValid()) {
            memcpy(((char**)param_value)[i], binary_[device]->full(),
                   binary_[device]->full_size());
          }
        }
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    case CL_PROGRAM_NUM_KERNELS: {
      pthread_mutex_lock(&mutex_build_);
      bool valid_program_executable = HasOneExecutable();
      size_t num_kernels = kernels_.size();
      pthread_mutex_unlock(&mutex_build_);

      if (!valid_program_executable) return CL_INVALID_PROGRAM_EXECUTABLE;
      size_t size = sizeof(size_t);
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        memcpy(param_value, &num_kernels, size);
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    case CL_PROGRAM_KERNEL_NAMES: {
      pthread_mutex_lock(&mutex_build_);
      bool valid_program_executable = HasOneExecutable();
      string concat_names;
      for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
           it != kernels_.end();
           ++it) {
        if (!concat_names.empty())
          concat_names.append(";");
        concat_names.append((*it)->name());
      }
      pthread_mutex_unlock(&mutex_build_);

      if (!valid_program_executable) return CL_INVALID_PROGRAM_EXECUTABLE;
      size_t size = concat_names.length() + 1;
      if (param_value) {
        if (param_value_size < size) return CL_INVALID_VALUE;
        strcpy((char*)param_value, concat_names.c_str());
      }
      if (param_value_size_ret) *param_value_size_ret = size;
      break;
    }
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLProgram::GetProgramBuildInfo(CLDevice* device,
                                      cl_program_build_info param_name,
                                      size_t param_value_size,
                                      void* param_value,
                                      size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_PROGRAM_BUILD_STATUS, cl_build_status,
                      build_status_[device]);
    GET_OBJECT_INFO_S(CL_PROGRAM_BUILD_OPTIONS, build_options_[device]);
    GET_OBJECT_INFO_S(CL_PROGRAM_BUILD_LOG, build_log_[device]);
    GET_OBJECT_INFO_T(CL_PROGRAM_BINARY_TYPE, cl_program_binary_type,
                      (binary_[device] == NULL ? CL_PROGRAM_BINARY_TYPE_NONE :
                                                 binary_[device]->type()));
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLProgram::BuildProgram(cl_uint num_devices,
                               const cl_device_id* device_list,
                               const char* options,
                               ProgramCallback* callback) {
  vector<CLDevice*> target_devices;
  if (num_devices > 0) {
    target_devices.reserve(num_devices);
    for (uint i = 0; i < num_devices; i++)
      target_devices.push_back(device_list[i]->c_obj);
  } else {
    target_devices = devices_;
  }

  if (!EnterBuild(target_devices))
    return CL_INVALID_OPERATION;

  if (callback != NULL)
    callback->set_count(target_devices.size());
  vector<CLEvent*> events;
  events.reserve(target_devices.size());
  for (vector<CLDevice*>::iterator it = target_devices.begin();
       it != target_devices.end();
       ++it) {
    CLDevice* device = *it;
    char* copied_options = NULL;
    if (options) {
      copied_options = (char*)malloc(sizeof(char) *
                                     (strlen(options) + 1));
      strcpy(copied_options, options);
    }
    ChangeBuildOptions(device, copied_options);
    ChangeBuildLog(device, NULL);
    build_callback_[device] = callback;

    CLEvent* event = device->EnqueueBuildProgram(this, source_,
                                                 binary_[device],
                                                 copied_options);
    events.push_back(event);
  }
  for (vector<CLEvent*>::iterator it = events.begin();
       it != events.end();
       ++it) {
    CLEvent* event = *it;
    if (callback == NULL)
      event->Wait();
    event->Release();
  }
  if (callback == NULL) {
    for (vector<CLDevice*>::iterator it = target_devices.begin();
         it != target_devices.end();
         ++it) {
      if (build_status_[*it] == CL_BUILD_ERROR)
        return CL_BUILD_PROGRAM_FAILURE;
    }
  }
  return CL_SUCCESS;
}

cl_int CLProgram::CompileProgram(cl_uint num_devices,
                                 const cl_device_id* device_list,
                                 const char* options,
                                 cl_uint num_input_headers,
                                 const cl_program* input_headers,
                                 const char** header_include_names,
                                 ProgramCallback* callback) {
  vector<CLDevice*> target_devices;
  if (num_devices > 0) {
    target_devices.reserve(num_devices);
    for (uint i = 0; i < num_devices; i++)
      target_devices.push_back(device_list[i]->c_obj);
  } else {
    target_devices = devices_;
  }

  if (!EnterBuild(target_devices))
    return CL_INVALID_OPERATION;

  if (callback != NULL)
    callback->set_count(target_devices.size());
  vector<CLEvent*> events;
  events.reserve(target_devices.size());
  for (vector<CLDevice*>::iterator it = target_devices.begin();
       it != target_devices.end();
       ++it) {
    CLDevice* device = *it;
    char* copied_options = NULL;
    if (options) {
      copied_options = (char*)malloc(sizeof(char) *
                                     (strlen(options) + 1));
      strcpy(copied_options, options);
    }
    ChangeBuildOptions(device, copied_options);
    ChangeBuildLog(device, NULL);
    build_callback_[device] = callback;

    vector<CLProgramSource*> headers;
    headers.reserve(num_input_headers);
    for (cl_uint i = 0; i < num_input_headers; i++) {
      if (input_headers[i]->c_obj->HasSource() &&
          header_include_names[i] != NULL) {
        bool already_exist = false;
        for (vector<CLProgramSource*>::iterator it = headers.begin();
             it != headers.end();
             ++it) {
          if (strcmp((*it)->header_name(), header_include_names[i]) == 0) {
            already_exist = true;
            break;
          }
        }
        if (already_exist)
          continue;

        CLProgramSource* header =
            new CLProgramSource(input_headers[i]->c_obj->source_);
        header->SetHeaderName(header_include_names[i]);
        headers.push_back(header);
      }
    }

    CLEvent* event = device->EnqueueCompileProgram(this, source_,
                                                   copied_options, headers);
    events.push_back(event);
  }
  for (vector<CLEvent*>::iterator it = events.begin();
       it != events.end();
       ++it) {
    CLEvent* event = *it;
    if (callback == NULL)
      event->Wait();
    event->Release();
  }
  if (callback == NULL) {
    for (vector<CLDevice*>::iterator it = target_devices.begin();
         it != target_devices.end();
         ++it) {
      if (build_status_[*it] == CL_BUILD_ERROR)
        return CL_BUILD_PROGRAM_FAILURE;
    }
  }
  return CL_SUCCESS;
}

cl_int CLProgram::LinkProgram(const char* options, cl_uint num_input_programs,
                              const cl_program* input_programs,
                              ProgramCallback* callback) {
  vector<CLDevice*> target_devices;
  target_devices.reserve(devices_.size());
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    CLDevice* device = *it;
    bool one_device_has_binary = false;
    bool all_devices_have_binary = true;
    for (cl_uint i = 0; i < num_input_programs; i++) {
      bool has_linkable_binary = true;
      CLProgram* input_program = input_programs[i]->c_obj;
      if (!input_program->IsValidDevice(device)) {
        has_linkable_binary = false;
      } else {
        CLProgramBinary* binary = input_program->binary_[device];
        if (binary == NULL || !binary->IsValid() ||
            (binary->type() != CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT &&
             binary->type() != CL_PROGRAM_BINARY_TYPE_LIBRARY))
          has_linkable_binary = false;
      }
      one_device_has_binary |= has_linkable_binary;
      all_devices_have_binary &= has_linkable_binary;
    }
    if (one_device_has_binary != all_devices_have_binary)
      return CL_INVALID_OPERATION;
    if (all_devices_have_binary)
      target_devices.push_back(device);
  }

  if (!EnterBuild(target_devices))
    return CL_INVALID_OPERATION;

  if (callback != NULL)
    callback->set_count(target_devices.size());
  vector<CLEvent*> events;
  events.reserve(target_devices.size());
  for (vector<CLDevice*>::iterator it = target_devices.begin();
       it != target_devices.end();
       ++it) {
    CLDevice* device = *it;
    char* copied_options = NULL;
    if (options) {
      copied_options = (char*)malloc(sizeof(char) *
                                     (strlen(options) + 1));
      strcpy(copied_options, options);
    }
    ChangeBuildOptions(device, copied_options);
    ChangeBuildLog(device, NULL);
    build_callback_[device] = callback;

    vector<CLProgramBinary*> binaries;
    binaries.reserve(num_input_programs);
    for (cl_uint i = 0; i < num_input_programs; i++) {
      CLProgramBinary* binary =
          new CLProgramBinary(input_programs[i]->c_obj->binary_[device]);
      binaries.push_back(binary);
    }

    CLEvent* event = device->EnqueueLinkProgram(this, binaries,
                                                copied_options);
    events.push_back(event);
  }
  for (vector<CLEvent*>::iterator it = events.begin();
       it != events.end();
       ++it) {
    CLEvent* event = *it;
    if (callback == NULL)
      event->Wait();
    event->Release();
  }
  if (callback == NULL) {
    for (vector<CLDevice*>::iterator it = target_devices.begin();
         it != target_devices.end();
         ++it) {
      if (build_status_[*it] == CL_BUILD_ERROR)
        return CL_BUILD_PROGRAM_FAILURE;
    }
  }
  return CL_SUCCESS;
}

CLKernel* CLProgram::CreateKernel(const char* kernel_name, cl_int* err) {
  pthread_mutex_lock(&mutex_build_);
  if (kernels_ref_cnt_ == -1) {
    pthread_mutex_unlock(&mutex_build_);
    *err = CL_INVALID_PROGRAM_EXECUTABLE;
    return NULL;
  }
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    CLKernelInfo* kernel_info = *it;
    if (strcmp(kernel_info->name(), kernel_name) == 0) {
      if (!kernel_info->IsValid()) {
        pthread_mutex_unlock(&mutex_build_);
        *err = CL_INVALID_KERNEL_DEFINITION;
        return NULL;
      }
      CLKernel* kernel = new CLKernel(context_, this, kernel_info);
      if (kernel == NULL) {
        pthread_mutex_unlock(&mutex_build_);
        *err = CL_OUT_OF_HOST_MEMORY;
        return NULL;
      }
      kernels_ref_cnt_++;
      pthread_mutex_unlock(&mutex_build_);
      return kernel;
    }
  }
  pthread_mutex_unlock(&mutex_build_);
  *err = CL_INVALID_KERNEL_NAME;
  return NULL;
}

cl_int CLProgram::CreateKernelsInProgram(cl_uint num_kernels,
                                         cl_kernel* kernels,
                                         cl_uint* num_kernels_ret) {
  if (kernels) {
    if (num_kernels < kernels_.size()) return CL_INVALID_VALUE;
    pthread_mutex_lock(&mutex_build_);
    if (kernels_ref_cnt_ == -1 || !HasAllExecutable()) {
      pthread_mutex_unlock(&mutex_build_);
      return CL_INVALID_PROGRAM_EXECUTABLE;
    }
    for (size_t i = 0; i < kernels_.size(); i++) {
      CLKernel* kernel = new CLKernel(context_, this, kernels_[i]);
      if (kernel == NULL) {
        pthread_mutex_unlock(&mutex_build_);
        for (size_t prev = 0; prev < i; prev++)
          kernels[prev]->c_obj->Release();
        return CL_OUT_OF_HOST_MEMORY;
      }
      kernels[i] = kernel->st_obj();
      kernels_ref_cnt_++;
    }
    pthread_mutex_unlock(&mutex_build_);
  }
  if (num_kernels_ret) *num_kernels_ret = kernels_.size();
  return CL_SUCCESS;
}

bool CLProgram::IsValidDevice(CLDevice* device) const {
  return (find(devices_.begin(), devices_.end(), device) != devices_.end());
}

bool CLProgram::IsValidDevices(cl_uint num_devices,
                               const cl_device_id* device_list) const {
  for (cl_uint i = 0; i < num_devices; i++) {
    if (device_list[i] == NULL || !IsValidDevice(device_list[i]->c_obj))
      return false;
  }
  return true;
}

bool CLProgram::HasSource() const {
  return (source_ != NULL);
}

bool CLProgram::HasValidBinary(CLDevice* device) {
  return (binary_[device] != NULL && binary_[device]->IsValid());
}

bool CLProgram::HasOneExecutable() const {
  return (build_success_cnt_ > 0);
}

bool CLProgram::HasAllExecutable() const {
  return (build_success_cnt_ == devices_.size());
}

bool CLProgram::IsCompilerAvailable() {
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    if (!((*it)->IsCompilerAvailable()))
      return false;
  }
  return true;
}

bool CLProgram::IsLinkerAvailable() {
  for (vector<CLDevice*>::iterator it = devices_.begin();
       it != devices_.end();
       ++it) {
    if (!((*it)->IsLinkerAvailable()))
      return false;
  }
  return true;
}

bool CLProgram::EnterBuild(CLDevice* target_device) {
  pthread_mutex_lock(&mutex_build_);
  if (kernels_ref_cnt_ > 0) {
    pthread_mutex_unlock(&mutex_build_);
    return false;
  }
  if (build_status_[target_device] == CL_BUILD_IN_PROGRESS) {
    pthread_mutex_unlock(&mutex_build_);
    return false;
  }
  if (executable_[target_device] != NULL)
    build_success_cnt_--;
  build_in_progress_cnt_++;
  build_status_[target_device] = CL_BUILD_IN_PROGRESS;
  if (build_success_cnt_ == 0)
    kernels_ref_cnt_ = -1;
  pthread_mutex_unlock(&mutex_build_);
  return true;
}

bool CLProgram::EnterBuild(const vector<CLDevice*>& target_devices) {
  pthread_mutex_lock(&mutex_build_);
  if (kernels_ref_cnt_ > 0) {
    pthread_mutex_unlock(&mutex_build_);
    return false;
  }
  for (vector<CLDevice*>::const_iterator it = target_devices.begin();
       it != target_devices.end();
       ++it) {
    if (build_status_[*it] == CL_BUILD_IN_PROGRESS) {
      pthread_mutex_unlock(&mutex_build_);
      return false;
    }
  }
  for (vector<CLDevice*>::const_iterator it = target_devices.begin();
       it != target_devices.end();
       ++it) {
    if (executable_[*it] != NULL)
      build_success_cnt_--;
    build_in_progress_cnt_++;
    build_status_[*it] = CL_BUILD_IN_PROGRESS;
  }
  if (build_success_cnt_ == 0)
    kernels_ref_cnt_ = -1;
  pthread_mutex_unlock(&mutex_build_);
  return true;
}

void CLProgram::CompleteBuild(CLDevice* device, cl_build_status status,
                              char* log, CLProgramBinary* binary,
                              void* executable) {
  if (status == CL_BUILD_SUCCESS) {
    ChangeBinary(device, binary);
    ChangeExecutable(device, executable);
  } else {
    ChangeBinary(device, NULL);
    ChangeExecutable(device, NULL);
  }
  ChangeBuildLog(device, log);

  pthread_mutex_lock(&mutex_build_);
  build_status_[device] = status;
  build_in_progress_cnt_--;
  if (status == CL_BUILD_SUCCESS)
    build_success_cnt_++;
  pthread_mutex_unlock(&mutex_build_);

  CountdownBuildCallback(device);
}

void CLProgram::CompleteBuild(CLDevice* device, cl_build_status status,
                              char* log, CLProgramBinary* binary) {
  if (status == CL_BUILD_SUCCESS)
    ChangeBinary(device, binary);
  else
    ChangeBinary(device, NULL);
  ChangeBuildLog(device, log);

  pthread_mutex_lock(&mutex_build_);
  build_status_[device] = status;
  build_in_progress_cnt_--;
  pthread_mutex_unlock(&mutex_build_);

  CountdownBuildCallback(device);
}

bool CLProgram::BeginRegisterKernelInfo() {
  pthread_mutex_lock(&mutex_build_);
  if (kernels_ref_cnt_ > 0) {
    pthread_mutex_unlock(&mutex_build_);
    return false;
  }
  if (kernels_ref_cnt_ == -1) {
    for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
         it != kernels_.end();
         ++it) {
      delete *it;
    }
    kernels_.clear();
  }
  return true;
}

void CLProgram::RegisterKernelInfo(
    const char* name, cl_uint num_args, const char* attributes,
    CLDevice* device, size_t work_group_size,
    size_t compile_work_group_size[3], cl_ulong local_mem_size,
    size_t preferred_work_group_size_multiple, cl_ulong private_mem_size,
    int snucl_index) {
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    CLKernelInfo* kernel_info = *it;
    if (strcmp(kernel_info->name(), name) == 0) {
      kernel_info->Update(name, num_args, attributes);
      kernel_info->SetWorkGroupInfo(device, work_group_size,
                                    compile_work_group_size, local_mem_size,
                                    preferred_work_group_size_multiple,
                                    private_mem_size);
      if (snucl_index >= 0)
        kernel_info->SetSnuCLIndex(snucl_index);
      return;
    }
  }
  CLKernelInfo* kernel_info = new CLKernelInfo(name, num_args, attributes);
  kernel_info->SetWorkGroupInfo(device, work_group_size,
                                compile_work_group_size, local_mem_size,
                                preferred_work_group_size_multiple,
                                private_mem_size);
  if (snucl_index >= 0)
    kernel_info->SetSnuCLIndex(snucl_index);
  kernels_.push_back(kernel_info);
}

void CLProgram::RegisterKernelArgInfo(
    const char* name, cl_uint arg_index,
    cl_kernel_arg_address_qualifier arg_address_qualifier,
    cl_kernel_arg_access_qualifier arg_access_qualifier,
    const char* arg_type_name, cl_kernel_arg_type_qualifier arg_type_qualifier,
    const char* arg_name) {
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    CLKernelInfo* kernel_info = *it;
    if (strcmp(kernel_info->name(), name) == 0) {
      kernel_info->SetArgInfo(arg_index, arg_address_qualifier,
                              arg_access_qualifier, arg_type_name,
                              arg_type_qualifier, arg_name);
      break;
    }
  }
}

void CLProgram::FinishRegisterKernelInfo() {
  if (kernels_ref_cnt_ == -1)
    kernels_ref_cnt_ = 0;
  pthread_mutex_unlock(&mutex_build_);
}

void CLProgram::ReleaseKernel() {
  pthread_mutex_lock(&mutex_build_);
  kernels_ref_cnt_--;
  pthread_mutex_unlock(&mutex_build_);
}

CLProgramBinary* CLProgram::GetBinary(CLDevice* device) {
  return binary_[device];
}

void* CLProgram::GetExecutable(CLDevice* device) {
  return executable_[device];
}

cl_build_status CLProgram::GetBuildStatus(CLDevice* device) {
  return build_status_[device];
}

const char* CLProgram::GetBuildLog(CLDevice* device) {
  return build_log_[device];
}

void* CLProgram::SerializeKernelInfo(CLDevice* device, size_t* size) {
  pthread_mutex_lock(&mutex_build_);
  if (kernels_.empty()) {
    pthread_mutex_unlock(&mutex_build_);
    *size = 0;
    return NULL;
  }
  *size = sizeof(size_t);
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    *size += (*it)->GetSerializationSize(device);
  }
  void* buffer = malloc(*size);
  void* p = buffer;
  size_t num_kernels = kernels_.size();
  memcpy(p, &num_kernels, sizeof(size_t));
  p = (void*)((char*)p + sizeof(size_t));
  for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
       it != kernels_.end();
       ++it) {
    p = (*it)->SerializeKernelInfo(p, device);
  }
  pthread_mutex_unlock(&mutex_build_);
  return buffer;
}

void CLProgram::DeserializeKernelInfo(void* buffer, CLDevice* device) {
  if (!BeginRegisterKernelInfo())
    return;
  char* p = (char*)buffer;

#define DESERIALIZE_INFO(type, value) \
  memcpy(&value, p, sizeof(type));    \
  p += sizeof(type);
#define DESERIALIZE_INFO_A(type, value, length) \
  memcpy(value, p, sizeof(type) * length);      \
  p += sizeof(type) * length;
#define DESERIALIZE_INFO_S(value) \
  {                               \
    size_t length = strlen(p);    \
    value = p;                    \
    p += length + 1;              \
  }

  size_t num_kernels;
  DESERIALIZE_INFO(size_t, num_kernels);
  while (num_kernels > 0) {
    num_kernels--;

    char* name;
    cl_uint num_args;
    char* attributes;
    size_t work_group_size;
    size_t compile_work_group_size[3];
    cl_ulong local_mem_size;
    size_t preferred_work_group_size_multiple;
    cl_ulong private_mem_size;
    DESERIALIZE_INFO_S(name);
    DESERIALIZE_INFO(cl_uint, num_args);
    DESERIALIZE_INFO_S(attributes);
    DESERIALIZE_INFO(size_t, work_group_size);
    DESERIALIZE_INFO_A(size_t, compile_work_group_size, 3);
    DESERIALIZE_INFO(cl_ulong, local_mem_size);
    DESERIALIZE_INFO(size_t, preferred_work_group_size_multiple);
    DESERIALIZE_INFO(cl_ulong, private_mem_size);
    RegisterKernelInfo(name, num_args, attributes, device, work_group_size,
                       compile_work_group_size, local_mem_size,
                       preferred_work_group_size_multiple, private_mem_size,
                       -1);

    while (true) {
      cl_uint i;
      cl_kernel_arg_address_qualifier arg_address_qualifier;
      cl_kernel_arg_access_qualifier arg_access_qualifier;
      char* arg_type_name;
      cl_kernel_arg_type_qualifier arg_type_qualifier;
      char* arg_name;
      DESERIALIZE_INFO(cl_uint, i);
      if (i >= num_args) break;
      DESERIALIZE_INFO(cl_kernel_arg_address_qualifier, arg_address_qualifier);
      DESERIALIZE_INFO(cl_kernel_arg_access_qualifier, arg_access_qualifier);
      DESERIALIZE_INFO_S(arg_type_name);
      DESERIALIZE_INFO(cl_kernel_arg_type_qualifier, arg_type_qualifier);
      DESERIALIZE_INFO_S(arg_name);
      RegisterKernelArgInfo(name, i, arg_address_qualifier,
                            arg_access_qualifier, arg_type_name,
                            arg_type_qualifier, arg_name);
    }

    bool valid;
    DESERIALIZE_INFO(bool, valid);
    if (!valid) {
      for (vector<CLKernelInfo*>::iterator it = kernels_.begin();
           it != kernels_.end();
           ++it) {
        CLKernelInfo* kernel_info = (*it);
        if (strcmp(kernel_info->name(), name) == 0) {
          kernel_info->Invalidate();
          break;
        }
      }
    }
  }
  FinishRegisterKernelInfo();
}

void CLProgram::ChangeBinary(CLDevice* device, CLProgramBinary* binary) {
  CLProgramBinary* old_binary = binary_[device];
  // Modified in soff-runtime
  if (old_binary != binary) {
    binary_[device] = binary;
    if (old_binary != NULL)
      delete old_binary;
  }
}

void CLProgram::ChangeExecutable(CLDevice* device, void* executable) {
  void* old_executable = executable_[device];
  executable_[device] = executable;
  if (old_executable != NULL)
    device->FreeExecutable(this, old_executable);
}

void CLProgram::ChangeBuildOptions(CLDevice* device, char* options) {
  char* old_options = build_options_[device];
  build_options_[device] = options;
  if (old_options != NULL)
    free(old_options);
}

void CLProgram::ChangeBuildLog(CLDevice* device, char* log) {
  char* old_log = build_log_[device];
  build_log_[device] = log;
  if (old_log != NULL)
    free(old_log);
}

void CLProgram::CountdownBuildCallback(CLDevice* device) {
  ProgramCallback* callback = build_callback_[device];
  build_callback_[device] = NULL;
  if (callback != NULL)
    callback->countdown(st_obj());
}

CLProgram*
CLProgram::CreateProgramWithSource(CLContext* context, cl_uint count,
                                   const char** strings, const size_t* lengths,
                                   cl_int* err) {
  CLProgram* program = new CLProgram(context, context->devices());
  if (program == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  program->source_ = new CLProgramSource();
  for (cl_uint i = 0; i < count; i++) {
    if (lengths == NULL || lengths[i] == 0)
      program->source_->AddSource(strings[i]);
    else
      program->source_->AddSource(strings[i], lengths[i]);
  }
  return program;
}

CLProgram*
CLProgram::CreateProgramWithBinary(CLContext* context, cl_uint num_devices,
                                   const cl_device_id* device_list,
                                   const size_t* lengths,
                                   const unsigned char** binaries,
                                   cl_int* binary_status, cl_int* err) {
  vector<CLDevice*> devices;
  devices.reserve(num_devices);
  for (cl_uint i = 0; i < num_devices; i++)
    devices.push_back(device_list[i]->c_obj);
  CLProgram* program = new CLProgram(context, devices);
  if (program == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  for (cl_uint i = 0; i < num_devices; i++)
    program->binary_[devices[i]] = new CLProgramBinary(devices[i], binaries[i],
                                                       lengths[i]);
  return program;
}

CLProgram*
CLProgram::CreateProgramWithBuiltInKernels(CLContext* context,
                                           cl_uint num_devices,
                                           const cl_device_id* device_list,
                                           const char* kernel_names,
                                           cl_int* err) {
  // Currently SnuCL does not support built-in kernels
  *err = CL_INVALID_VALUE;
  return NULL;
}

CLProgram*
CLProgram::CreateProgramWithNothing(CLContext* context, cl_uint num_devices,
                                    const cl_device_id* device_list,
                                    cl_int* err) {
  vector<CLDevice*> devices;
  devices.reserve(num_devices);
  for (cl_uint i = 0; i < num_devices; i++)
    devices.push_back(device_list[i]->c_obj);
  CLProgram* program = new CLProgram(context, devices);
  if (program == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  return program;
}
