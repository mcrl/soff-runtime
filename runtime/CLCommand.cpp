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

#include "CLCommand.h"
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <malloc.h>
#include <CL/cl.h>
#include <CL/cl_ext_snucl.h>
#include "Callbacks.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLDevice.h"
#include "CLEvent.h"
#include "CLFile.h"
#include "CLKernel.h"
#include "CLMem.h"
#include "CLProgram.h"
#include "CLSampler.h"
#include "Structs.h"

using namespace std;

CLCommand::CLCommand(CLContext* context, CLDevice* device,
                     CLCommandQueue* queue, cl_command_type type) {
  type_ = type;
  queue_ = queue;
  if (queue_ != NULL) {
    queue_->Retain();
    context = queue_->context();
    device = queue_->device();
  }
  context_ = context;
  context_->Retain();
  device_ = device;
  if (queue_ != NULL) {
    event_ = new CLEvent(queue, this);
  } else {
    event_ = new CLEvent(context, this);
  }
  wait_events_complete_ = false;
  wait_events_good_ = true;
  consistency_resolved_ = false;
  error_ = CL_SUCCESS;

  dev_src_ = NULL;
  dev_dst_ = NULL;
  node_src_ = -1;
  node_dst_ = -1;
  event_id_ = event_->id();

  mem_src_ = NULL;
  mem_dst_ = NULL;
  pattern_ = NULL;
  kernel_ = NULL;
  kernel_args_ = NULL;
  native_args_ = NULL;
  mem_list_ = NULL;
  mem_offsets_ = NULL;
  temp_buf_ = NULL;
  program_ = NULL;
  headers_ = NULL;
  link_binaries_ = NULL;

  file_src_ = NULL;
  file_dst_ = NULL;

  custom_function_ = NULL;
  custom_data_ = NULL;
}

CLCommand::~CLCommand() {
  if (queue_) queue_->Release();
  context_->Release();
  for (vector<CLEvent*>::iterator it = wait_events_.begin();
       it != wait_events_.end();
       ++it) {
    (*it)->Release();
  }
  if (mem_src_) mem_src_->Release();
  if (mem_dst_) mem_dst_->Release();
  if (pattern_) free(pattern_);
  if (kernel_) kernel_->Release();
  if (kernel_args_) {
    for (map<cl_uint, CLKernelArg*>::iterator it = kernel_args_->begin();
         it != kernel_args_->end();
         ++it) {
      CLKernelArg* arg = it->second;
      if (arg->mem) arg->mem->Release();
      if (arg->sampler) arg->sampler->Release();
      free(arg);
    }
    delete kernel_args_;
  }
  if (native_args_) free(native_args_);
  if (mem_list_) {
    for (uint i = 0; i < num_mem_objects_; i++)
      mem_list_[i]->Release();
    free(mem_list_);
  }
  if (mem_offsets_) free(mem_offsets_);
  if (temp_buf_) free(temp_buf_);
  if (program_) program_->Release();
  if (headers_) {
    for (size_t i = 0; i < size_; i++)
      delete headers_[i];
    free(headers_);
  }
  if (link_binaries_) {
    for (size_t i = 0; i < size_; i++)
      delete link_binaries_[i];
    free(link_binaries_);
  }
  if (file_src_) file_src_->Release();
  if (file_dst_) file_dst_->Release();
  event_->Release();
}

void CLCommand::SetWaitList(cl_uint num_events_in_wait_list,
                            const cl_event* event_wait_list) {
  wait_events_.reserve(num_events_in_wait_list);
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    CLEvent* e = event_wait_list[i]->c_obj;
    e->Retain();
    wait_events_.push_back(e);
  }
}

void CLCommand::AddWaitEvent(CLEvent* event) {
  event->Retain();
  wait_events_.push_back(event);
  wait_events_complete_ = false;
}

bool CLCommand::IsExecutable() {
  if (wait_events_complete_)
    return true;

  for (vector<CLEvent*>::iterator it = wait_events_.begin();
       it != wait_events_.end();
       ++it) {
    if (!(*it)->IsComplete())
      return false;
  }

  for (vector<CLEvent*>::iterator it = wait_events_.begin();
       it != wait_events_.end();
       ++it) {
    CLEvent* event = *it;
    if (event->IsError())
      wait_events_good_ = false;
    event->Release();
  }
  wait_events_.clear();
  wait_events_complete_ = true;
  return true;
}

void CLCommand::Submit() {
  event_->SetStatus(CL_SUBMITTED);
  device_->EnqueueReadyQueue(this);
}

void CLCommand::SetError(cl_int error) {
  error_ = error;
}

void CLCommand::SetAsRunning() {
  event_->SetStatus(CL_RUNNING);
}

void CLCommand::SetAsComplete() {
  if (error_ != CL_SUCCESS && error_ < 0) {
    event_->SetStatus(error_);
  } else {
    event_->SetStatus(CL_COMPLETE);
  }
}

CLEvent* CLCommand::ExportEvent() {
  event_->Retain();
  return event_;
}

void CLCommand::AnnotateSourceDevice(CLDevice* device) {
  dev_src_ = device;
}

void CLCommand::AnnotateDestinationDevice(CLDevice* device) {
  dev_dst_ = device;
}

void CLCommand::AnnotateSourceNode(int node) {
  node_src_ = node;
}

void CLCommand::AnnotateDestinationNode(int node) {
  node_dst_ = node;
}

bool CLCommand::IsPartialCommand() const {
  return event_id_ != event_->id();
}

void CLCommand::SetAsPartialCommand(CLCommand* root) {
  event_id_ = root->event_id_;
}

void CLCommand::Execute() {
  switch (type_) {
    case CL_COMMAND_NDRANGE_KERNEL:
    case CL_COMMAND_TASK:
      device_->LaunchKernel(this, kernel_, work_dim_, gwo_, gws_, lws_, nwg_,
                            kernel_args_);
      break;
    case CL_COMMAND_NATIVE_KERNEL:
      device_->LaunchNativeKernel(this, user_func_, native_args_, size_,
                                  num_mem_objects_, mem_list_, mem_offsets_);
      break;
    case CL_COMMAND_READ_BUFFER:
      device_->ReadBuffer(this, mem_src_, off_src_, size_, ptr_);
      break;
    case CL_COMMAND_WRITE_BUFFER:
      device_->WriteBuffer(this, mem_dst_, off_dst_, size_, ptr_);
      break;
    case CL_COMMAND_COPY_BUFFER:
      device_->CopyBuffer(this, mem_src_, mem_dst_, off_src_, off_dst_, size_);
      break;
    case CL_COMMAND_READ_IMAGE:
      device_->ReadImage(this, mem_src_, src_origin_, region_, dst_row_pitch_,
                         dst_slice_pitch_, ptr_);
      break;
    case CL_COMMAND_WRITE_IMAGE:
      device_->WriteImage(this, mem_dst_, dst_origin_, region_, src_row_pitch_,
                          src_slice_pitch_, ptr_);
      break;
    case CL_COMMAND_COPY_IMAGE:
      device_->CopyImage(this, mem_src_, mem_dst_, src_origin_, dst_origin_,
                         region_);
      break;
    case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
      device_->CopyImageToBuffer(this, mem_src_, mem_dst_, src_origin_,
                                 region_, off_dst_);
      break;
    case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
      device_->CopyBufferToImage(this, mem_src_, mem_dst_, off_src_,
                                 dst_origin_, region_);
      break;
    case CL_COMMAND_MAP_BUFFER:
      device_->MapBuffer(this, mem_src_, map_flags_, off_src_, size_, ptr_);
      break;
    case CL_COMMAND_MAP_IMAGE:
      device_->MapImage(this, mem_src_, map_flags_, src_origin_, region_,
                        ptr_);
      break;
    case CL_COMMAND_UNMAP_MEM_OBJECT:
      device_->UnmapMemObject(this, mem_src_, ptr_);
      break;
    case CL_COMMAND_MARKER:
      break;
    case CL_COMMAND_READ_BUFFER_RECT:
      device_->ReadBufferRect(this, mem_src_, src_origin_, dst_origin_,
                              region_, src_row_pitch_, src_slice_pitch_,
                              dst_row_pitch_, dst_slice_pitch_, ptr_);
      break;
    case CL_COMMAND_WRITE_BUFFER_RECT:
      device_->WriteBufferRect(this, mem_dst_, src_origin_, dst_origin_,
                               region_, src_row_pitch_, src_slice_pitch_,
                               dst_row_pitch_, dst_slice_pitch_, ptr_);
      break;
    case CL_COMMAND_COPY_BUFFER_RECT:
      device_->CopyBufferRect(this, mem_src_, mem_dst_, src_origin_,
                              dst_origin_, region_, src_row_pitch_,
                              src_slice_pitch_, dst_row_pitch_,
                              dst_slice_pitch_);
      break;
    case CL_COMMAND_BARRIER:
      break;
    case CL_COMMAND_MIGRATE_MEM_OBJECTS:
      device_->MigrateMemObjects(this, num_mem_objects_, mem_list_,
                                 migration_flags_);
      break;
    case CL_COMMAND_FILL_BUFFER:
      device_->FillBuffer(this, mem_dst_, pattern_, pattern_size_, off_dst_,
                          size_);
      break;
    case CL_COMMAND_FILL_IMAGE:
      device_->FillImage(this, mem_dst_, ptr_, dst_origin_, region_);
      break;
    case CL_COMMAND_BUILD_PROGRAM:
      device_->BuildProgram(this, program_, source_, binary_, options_);
      break;
    case CL_COMMAND_COMPILE_PROGRAM:
      device_->CompileProgram(this, program_, source_, options_, size_,
                              headers_);
      break;
    case CL_COMMAND_LINK_PROGRAM:
      device_->LinkProgram(this, program_, size_, link_binaries_, options_);
      break;
    case CL_COMMAND_WAIT_FOR_EVENTS:
      break;
    case CL_COMMAND_CUSTOM:
      custom_function_(custom_data_);
      break;
    case CL_COMMAND_NOP:
      break;
    case CL_COMMAND_ALLTOALL_BUFFER:
      device_->AlltoAllBuffer(this, mem_src_, mem_dst_, off_src_, off_dst_,
                              size_);
      break;
    case CL_COMMAND_BROADCAST_BUFFER:
      device_->BroadcastBuffer(this, mem_src_, mem_dst_, off_src_, off_dst_,
                               size_);
      break;
    case CL_COMMAND_LOCAL_FILE_OPEN:
      device_->LocalFileOpen(this, file_dst_, filename_, file_open_flags_);
      break;
    case CL_COMMAND_COPY_BUFFER_TO_FILE:
      device_->CopyBufferToFile(this, mem_src_, file_dst_, off_src_, off_dst_,
                                size_);
      break;
    case CL_COMMAND_COPY_FILE_TO_BUFFER:
      device_->CopyFileToBuffer(this, file_src_, mem_dst_, off_src_, off_dst_,
                                size_);
      break;
    default:
      SNUCL_ERROR("Unsupported command [%x]", type_);
      break;
  }
}

bool CLCommand::ResolveConsistency() {
  bool resolved = consistency_resolved_;
  if (!resolved) {
    switch (type_) {
      case CL_COMMAND_NDRANGE_KERNEL:
      case CL_COMMAND_TASK:
        resolved = ResolveConsistencyOfLaunchKernel();
        break;
      case CL_COMMAND_NATIVE_KERNEL:
        resolved = ResolveConsistencyOfLaunchNativeKernel();
        break;
      case CL_COMMAND_READ_BUFFER:
      case CL_COMMAND_READ_IMAGE:
      case CL_COMMAND_READ_BUFFER_RECT:
        resolved = ResolveConsistencyOfReadMem();
        break;
      case CL_COMMAND_WRITE_BUFFER:
      case CL_COMMAND_WRITE_IMAGE:
      case CL_COMMAND_WRITE_BUFFER_RECT:
      case CL_COMMAND_FILL_BUFFER:
      case CL_COMMAND_FILL_IMAGE:
        resolved = ResolveConsistencyOfWriteMem();
        break;
      case CL_COMMAND_COPY_BUFFER:
      case CL_COMMAND_COPY_IMAGE:
      case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
      case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
      case CL_COMMAND_COPY_BUFFER_RECT:
        resolved = ResolveConsistencyOfCopyMem();
        break;
      case CL_COMMAND_MAP_BUFFER:
      case CL_COMMAND_MAP_IMAGE:
        resolved = ResolveConsistencyOfMap();
        break;
      case CL_COMMAND_UNMAP_MEM_OBJECT:
        resolved = ResolveConsistencyOfUnmap();
        break;
      case CL_COMMAND_BROADCAST_BUFFER:
        resolved = ResolveConsistencyOfBroadcast();
        break;
      case CL_COMMAND_ALLTOALL_BUFFER:
        resolved = ResolveConsistencyOfAlltoAll();
        break;
      case CL_COMMAND_COPY_BUFFER_TO_FILE:
        resolved = ResolveConsistencyOfCopyMemToFile();
        break;
      case CL_COMMAND_COPY_FILE_TO_BUFFER:
        resolved = ResolveConsistencyOfCopyFileToMem();
        break;
      default:
        resolved = true;
        break;
    }
  }
  if (resolved) {
    switch (type_) {
      case CL_COMMAND_NDRANGE_KERNEL:
      case CL_COMMAND_TASK:
        UpdateConsistencyOfLaunchKernel();
        break;
      case CL_COMMAND_NATIVE_KERNEL:
        UpdateConsistencyOfLaunchNativeKernel();
        break;
      case CL_COMMAND_READ_BUFFER:
      case CL_COMMAND_READ_IMAGE:
      case CL_COMMAND_READ_BUFFER_RECT:
        UpdateConsistencyOfReadMem();
        break;
      case CL_COMMAND_WRITE_BUFFER:
      case CL_COMMAND_WRITE_IMAGE:
      case CL_COMMAND_WRITE_BUFFER_RECT:
      case CL_COMMAND_FILL_BUFFER:
      case CL_COMMAND_FILL_IMAGE:
        UpdateConsistencyOfWriteMem();
        break;
      case CL_COMMAND_COPY_BUFFER:
      case CL_COMMAND_COPY_IMAGE:
      case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
      case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
      case CL_COMMAND_COPY_BUFFER_RECT:
        UpdateConsistencyOfCopyMem();
        break;
      case CL_COMMAND_MAP_BUFFER:
      case CL_COMMAND_MAP_IMAGE:
        UpdateConsistencyOfMap();
        break;
      case CL_COMMAND_UNMAP_MEM_OBJECT:
        UpdateConsistencyOfUnmap();
        break;
      case CL_COMMAND_BROADCAST_BUFFER:
        UpdateConsistencyOfBroadcast();
        break;
      case CL_COMMAND_ALLTOALL_BUFFER:
        UpdateConsistencyOfAlltoAll();
        break;
      case CL_COMMAND_COPY_BUFFER_TO_FILE:
        UpdateConsistencyOfCopyMemToFile();
        break;
      case CL_COMMAND_COPY_FILE_TO_BUFFER:
        UpdateConsistencyOfCopyFileToMem();
        break;
      default:
        break;
    }
  }
  return resolved;
}

bool CLCommand::ResolveConsistencyOfLaunchKernel() {
  bool already_resolved = true;
  for (map<cl_uint, CLKernelArg*>::iterator it = kernel_args_->begin();
       it != kernel_args_->end();
       ++it) {
    CLKernelArg* arg = it->second;
    if (arg->mem != NULL)
      already_resolved &= LocateMemOnDevice(arg->mem);
  }
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfLaunchNativeKernel() {
  bool already_resolved = true;
  for (cl_uint i = 0; i < num_mem_objects_; i++)
    already_resolved &= LocateMemOnDevice(mem_list_[i]);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfReadMem() {
  bool already_resolved = ChangeDeviceToReadMem(mem_src_, device_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfWriteMem() {
  bool already_resolved = true;
  bool write_all = false;
  switch (type_) {
    case CL_COMMAND_WRITE_BUFFER:
      write_all = (off_dst_ == 0 && size_ == mem_dst_->size());
      break;
    case CL_COMMAND_WRITE_IMAGE: {
      size_t* region = mem_dst_->GetImageRegion();
      write_all = (dst_origin_[0] == 0 && dst_origin_[1] == 0 &&
                   dst_origin_[2] == 0 && region_[0] == region[0] &&
                   region_[1] == region[1] && region_[2] == region[2]);
      break;
    }
    case CL_COMMAND_WRITE_BUFFER_RECT:
      write_all = (dst_origin_[0] == 0 && dst_origin_[1] == 0 &&
                   dst_origin_[2] == 0 &&
                   region_[0] * region_[1] * region_[2] == mem_dst_->size() &&
                   (dst_row_pitch_ == 0 || dst_row_pitch_ == region_[0]) &&
                   (dst_slice_pitch_ == 0 ||
                    dst_slice_pitch_ == region_[0] * region_[1]));
      break;
    case CL_COMMAND_FILL_BUFFER:
    case CL_COMMAND_FILL_IMAGE:
      write_all = true;
      break;
    default:
      SNUCL_ERROR("Unsupported command [%x]", type_);
      break;
  }
  if (!write_all)
    already_resolved &= LocateMemOnDevice(mem_dst_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfCopyMem() {
  CLDevice* source = device_;
  if (!ChangeDeviceToReadMem(mem_src_, source))
    return false;

  bool already_resolved = true;
  bool write_all = false;
  switch (type_) {
    case CL_COMMAND_COPY_BUFFER:
      write_all = (off_dst_ == 0 && size_ == mem_dst_->size());
      break;
    case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
      write_all = (off_dst_ == 0 &&
                   region_[0] * region_[1] * region_[2] == mem_dst_->size());
      break;
    case CL_COMMAND_COPY_IMAGE:
    case CL_COMMAND_COPY_BUFFER_TO_IMAGE: {
      size_t* region = mem_dst_->GetImageRegion();
      write_all = (dst_origin_[0] == 0 && dst_origin_[1] == 0 &&
                   dst_origin_[2] == 0 && region_[0] == region[0] &&
                   region_[1] == region[1] && region_[2] == region[2]);
      break;
    }
    case CL_COMMAND_COPY_BUFFER_RECT:
      write_all = (dst_origin_[0] == 0 && dst_origin_[1] == 0 &&
                   dst_origin_[2] == 0 &&
                   region_[0] * region_[1] * region_[2] == mem_dst_->size() &&
                   (dst_row_pitch_ == 0 || dst_row_pitch_ == region_[0]) &&
                   (dst_slice_pitch_ == 0 ||
                    dst_slice_pitch_ == region_[0] * region_[1]));
      break;
    default:
      SNUCL_ERROR("Unsupported command [%x]", type_);
      break;
  }
  if (!write_all)
    already_resolved &= LocateMemOnDevice(mem_dst_);

  bool use_read, use_write, use_copy, use_send, use_recv, use_rcopy;
  bool alloc_ptr, use_host_ptr;
  GetCopyPattern(source, device_, use_read, use_write, use_copy, use_send,
                 use_recv, use_rcopy, alloc_ptr, use_host_ptr /* unused */);

  void* ptr = NULL;
  if (alloc_ptr) {
    size_t size;
    switch (type_) {
      case CL_COMMAND_COPY_BUFFER:
        size = size_;
        break;
      case CL_COMMAND_COPY_IMAGE:
      case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
      case CL_COMMAND_COPY_BUFFER_RECT:
        size = mem_dst_->GetRegionSize(region_);
        break;
      case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
        size = mem_src_->GetRegionSize(region_);
        break;
      default:
        SNUCL_ERROR("Unsupported command [%x]", type_);
        break;
    }
    ptr = memalign(4096, size);
  }

  CLCommand* read = NULL;
  switch (type_) {
    case CL_COMMAND_COPY_BUFFER:
      if (use_read || use_send) {
        read = CreateReadBuffer(context_, source, NULL, mem_src_, off_src_,
                                size_, ptr);
      }
      if (use_write || use_recv) {
        type_ = CL_COMMAND_WRITE_BUFFER;
        ptr_ = ptr;
      }
      break;
    case CL_COMMAND_COPY_IMAGE:
      if (use_read || use_send) {
        read = CreateReadImage(context_, source, NULL, mem_src_, src_origin_,
                               region_, 0, 0, ptr);
      }
      if (use_write || use_recv) {
        type_ = CL_COMMAND_WRITE_IMAGE;
        src_row_pitch_ = 0;
        src_slice_pitch_ = 0;
        ptr_ = ptr;
      }
      break;
    case CL_COMMAND_COPY_IMAGE_TO_BUFFER:
      if (use_read || use_send) {
        read = CreateReadImage(context_, source, NULL, mem_src_, src_origin_,
                               region_, 0, 0, ptr);
      }
      if (use_write || use_recv) {
        type_ = CL_COMMAND_WRITE_BUFFER;
        size_ = mem_src_->GetRegionSize(region_);
        ptr_ = ptr;
      }
      break;
    case CL_COMMAND_COPY_BUFFER_TO_IMAGE:
      if (use_read || use_send) {
        read = CreateReadBuffer(context_, source, NULL, mem_src_, off_src_,
                                mem_src_->GetRegionSize(region_), ptr);
      }
      if (use_write || use_recv) {
        type_ = CL_COMMAND_WRITE_IMAGE;
        src_row_pitch_ = 0;
        src_slice_pitch_ = 0;
        ptr_ = ptr;
      }
      break;
    case CL_COMMAND_COPY_BUFFER_RECT:
      if (use_read || use_send) {
        size_t host_origin[3] = {0, 0, 0};
        read = CreateReadBufferRect(context_, source, NULL, mem_src_,
                                    src_origin_, host_origin, region_,
                                    src_row_pitch_, src_slice_pitch_, 0, 0,
                                    ptr);
      }
      if (use_write || use_recv) {
        type_ = CL_COMMAND_WRITE_BUFFER_RECT;
        src_origin_[0] = 0;
        src_origin_[1] = 0;
        src_origin_[2] = 0;
        src_row_pitch_ = 0;
        src_slice_pitch_ = 0;
        ptr_ = ptr;
      }
      break;
    default:
      SNUCL_ERROR("Unsupported command [%x]", type_);
      break;
  }
  if (use_send) {
    read->AnnotateDestinationNode(device_->node_id());
    read->SetAsPartialCommand(this);
  }
  if (use_recv)
    AnnotateSourceNode(source->node_id());
  if (use_rcopy) {
    AnnotateSourceDevice(source);
    AnnotateDestinationDevice(device_);
  }
  if (alloc_ptr)
    temp_buf_ = ptr;

  if (use_read && use_write) {
    CLEvent* last_event = read->ExportEvent();
    AddWaitEvent(last_event);
    last_event->Release();
    already_resolved = false;
  }
  if (read != NULL)
    read->Submit();
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfMap() {
  bool already_resolved = LocateMemOnDevice(mem_src_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfUnmap() {
  bool already_resolved = LocateMemOnDevice(mem_src_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfBroadcast() {
  bool already_resolved = true;
  bool write_all = (off_dst_ == 0 && size_ == mem_dst_->size());
  if (!(off_dst_ == 0 && size_ == mem_dst_->size()))
    already_resolved &= LocateMemOnDevice(mem_dst_);
  // To be reimplemented
  AnnotateSourceDevice(mem_src_->FrontLatest());
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfAlltoAll() {
  bool already_resolved = true;
  already_resolved &= LocateMemOnDevice(mem_src_);
  already_resolved &= LocateMemOnDevice(mem_dst_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfCopyMemToFile() {
  bool already_resolved = true;
  already_resolved &= LocateMemOnDevice(mem_src_);
  consistency_resolved_ = true;
  return already_resolved;
}

bool CLCommand::ResolveConsistencyOfCopyFileToMem() {
  bool already_resolved = true;
  already_resolved &= LocateMemOnDevice(mem_dst_);
  consistency_resolved_ = true;
  return already_resolved;
}

void CLCommand::UpdateConsistencyOfLaunchKernel() {
  for (map<cl_uint, CLKernelArg*>::iterator it = kernel_args_->begin();
       it != kernel_args_->end();
       ++it) {
    CLKernelArg* arg = it->second;
    if (arg->mem != NULL)
      AccessMemOnDevice(arg->mem, arg->mem->IsWritable());
  }
}

void CLCommand::UpdateConsistencyOfLaunchNativeKernel() {
  for (cl_uint i = 0; i < num_mem_objects_; i++)
    AccessMemOnDevice(mem_list_[i], true);
}

void CLCommand::UpdateConsistencyOfReadMem() {
  AccessMemOnDevice(mem_src_, false);
}

void CLCommand::UpdateConsistencyOfWriteMem() {
  AccessMemOnDevice(mem_dst_, true);
}

void CLCommand::UpdateConsistencyOfCopyMem() {
  AccessMemOnDevice(mem_dst_, true);
}

void CLCommand::UpdateConsistencyOfMap() {
  AccessMemOnDevice(mem_src_, false);
}

void CLCommand::UpdateConsistencyOfUnmap() {
  AccessMemOnDevice(mem_src_, true);
}

void CLCommand::UpdateConsistencyOfBroadcast() {
  AccessMemOnDevice(mem_dst_, true);
}

void CLCommand::UpdateConsistencyOfAlltoAll() {
  AccessMemOnDevice(mem_dst_, true);
}

void CLCommand::UpdateConsistencyOfCopyMemToFile() {
  AccessMemOnDevice(mem_src_, false);
}

void CLCommand::UpdateConsistencyOfCopyFileToMem() {
  AccessMemOnDevice(mem_dst_, true);
}

void CLCommand::GetCopyPattern(CLDevice* dev_src, CLDevice* dev_dst,
                               bool& use_read, bool& use_write, bool& use_copy,
                               bool& use_send, bool& use_recv, bool& use_rcopy,
                               bool& alloc_ptr, bool& use_host_ptr) {
  /*
   * GetCopyPattern() decides a method to copy a memory object in dev_src to a
   * memory object in dev_dst. Possible results are as follows:
   *
   * (1) dev_dst -> Copy -> dev_dst (dev_src == dev_dst)
   * (2) host pointer -> Write -> dev_dst
   * (3) dev_src -> Read -> a temporary buffer in host -> Write -> dev_dst
   * (4) dev_src -> ClusterDriver -> dev_dst
   * (5) dev_src -> Read -> MPI_Send -> MPI_Recv -> Write -> dev_dst
   *
   * No.  Required Commands                     Intermediate Buffer
   *      read  write  copy  send  recv  rcopy  alloc  use_host
   * (1)               TRUE
   * (2)        TRUE                                   TRUE
   * (3)  TRUE  TRUE                            TRUE
   * (4)                                 TRUE
   * (5)                     TRUE  TRUE
   */

  use_read = false;
  use_write = false;
  use_copy = false;
  use_send = false;
  use_recv = false;
  use_rcopy = false;
  alloc_ptr = false;
  use_host_ptr = false;

  if (dev_src == dev_dst) { // (1)
    use_copy = true;
  } else if (dev_src == LATEST_HOST) { // (2)
    use_write = true;
    use_host_ptr = true;
  } else if (dev_src->node_id() == dev_dst->node_id()) {
    if (dev_src->node_id() == 0) { // (3)
      use_read = true;
      use_write = true;
      alloc_ptr = true;
    } else { // (4)
      use_rcopy = true;
    }
  } else { // (5)
    use_send = true;
    use_recv = true;
  }
}

static void CL_CALLBACK IssueCommandCallback(cl_event event, cl_int status,
                                             void* user_data) {
  CLCommand* command = (CLCommand*)user_data;
  command->Submit();
}

CLEvent* CLCommand::CloneMem(CLDevice* dev_src, CLDevice* dev_dst,
                             CLMem* mem) {
  bool use_read, use_write, use_copy, use_send, use_recv, use_rcopy;
  bool alloc_ptr, use_host_ptr;
  GetCopyPattern(dev_src, dev_dst, use_read, use_write, use_copy, use_send,
                 use_recv, use_rcopy, alloc_ptr, use_host_ptr);

  void* ptr = NULL;
  if (alloc_ptr)
    ptr = memalign(4096, mem->size());
  if (use_host_ptr)
    ptr = mem->GetHostPtr();

  CLCommand* read = NULL;
  CLCommand* write = NULL;
  CLCommand* copy = NULL;
  if (mem->IsImage()) {
    size_t origin[3] = {0, 0, 0};
    size_t* region = mem->GetImageRegion();
    if (use_read || use_send)
      read = CreateReadImage(context_, dev_src, NULL, mem, origin, region, 0,
                             0, ptr);
    if (use_write || use_recv)
      write = CreateWriteImage(context_, dev_dst, NULL, mem, origin, region, 0,
                               0, ptr);
    if (use_copy || use_rcopy)
      copy = CreateCopyImage(context_, dev_dst, NULL, mem, mem, origin, origin,
                             region);
  } else {
    if (use_read || use_send)
      read = CreateReadBuffer(context_, dev_src, NULL, mem, 0, mem->size(),
                              ptr);
    if (use_write || use_recv)
      write = CreateWriteBuffer(context_, dev_dst, NULL, mem, 0, mem->size(),
                                ptr);
    if (use_copy || use_rcopy)
      copy = CreateCopyBuffer(context_, dev_dst, NULL, mem, mem, 0, 0,
                              mem->size());
  }
  if (use_send) {
    read->AnnotateDestinationNode(dev_dst->node_id());
    read->SetAsPartialCommand(write);
  }
  if (use_recv)
    write->AnnotateSourceNode(dev_src->node_id());
  if (use_rcopy) {
    copy->AnnotateSourceDevice(dev_src);
    copy->AnnotateDestinationDevice(dev_dst);
  }
  if (alloc_ptr)
    write->temp_buf_ = ptr;

  CLEvent* last_event = NULL;
  if (copy != NULL)
    last_event = copy->ExportEvent();
  else
    last_event = write->ExportEvent();

  if (use_read && use_write) {
    read->event_->AddCallback(new EventCallback(IssueCommandCallback, write,
                                                CL_COMPLETE));
    write = NULL;
  }
  if (read != NULL)
    read->Submit();
  if (write != NULL)
    write->Submit();
  if (copy != NULL)
    copy->Submit();
  mem->AddLatest(dev_dst);

  return last_event;
}

bool CLCommand::LocateMemOnDevice(CLMem* mem) {
  if (mem->HasLatest(device_) || mem->EmptyLatest())
    return true;
  CLDevice* source = mem->GetNearestLatest(device_);
  CLEvent* last_event = CloneMem(source, device_, mem);
  AddWaitEvent(last_event);
  last_event->Release();
  return false;
}

void CLCommand::AccessMemOnDevice(CLMem* mem, bool write) {
  if (write)
    mem->SetLatest(device_);
  else
    mem->AddLatest(device_);
}

bool CLCommand::ChangeDeviceToReadMem(CLMem* mem, CLDevice*& device) {
  if (mem->HasLatest(device) || mem->EmptyLatest())
    return true;
  CLDevice* source = mem->GetNearestLatest(device);
  if (source == LATEST_HOST) {
    CLEvent* last_event = CloneMem(source, device, mem);
    AddWaitEvent(last_event);
    last_event->Release();
    return false;
  }
  device = source;
  return true;
}

CLCommand*
CLCommand::CreateReadBuffer(CLContext* context, CLDevice* device,
                            CLCommandQueue* queue, CLMem* buffer,
                            size_t offset, size_t size, void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_READ_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = buffer;
  command->mem_src_->Retain();
  command->off_src_ = offset;
  command->size_ = size;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateReadBufferRect(CLContext* context, CLDevice* device,
                                CLCommandQueue* queue, CLMem* buffer,
                                const size_t* buffer_origin,
                                const size_t* host_origin,
                                const size_t* region,
                                size_t buffer_row_pitch,
                                size_t buffer_slice_pitch,
                                size_t host_row_pitch, size_t host_slice_pitch,
                                void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_READ_BUFFER_RECT);
  if (command == NULL) return NULL;
  command->mem_src_ = buffer;
  command->mem_src_->Retain();
  memcpy(command->src_origin_, buffer_origin, sizeof(size_t) * 3);
  memcpy(command->dst_origin_, host_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->src_row_pitch_ = buffer_row_pitch;
  command->src_slice_pitch_ = buffer_slice_pitch;
  command->dst_row_pitch_ = host_row_pitch;
  command->dst_slice_pitch_ = host_slice_pitch;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateWriteBuffer(CLContext* context, CLDevice* device,
                             CLCommandQueue* queue, CLMem* buffer,
                             size_t offset, size_t size, void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_WRITE_BUFFER);
  if (command == NULL) return NULL;
  command->mem_dst_ = buffer;
  command->mem_dst_->Retain();
  command->off_dst_ = offset;
  command->size_ = size;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateWriteBufferRect(CLContext* context, CLDevice* device,
                                 CLCommandQueue* queue, CLMem* buffer,
                                 const size_t* buffer_origin,
                                 const size_t* host_origin,
                                 const size_t* region,
                                 size_t buffer_row_pitch,
                                 size_t buffer_slice_pitch,
                                 size_t host_row_pitch,
                                 size_t host_slice_pitch, void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_WRITE_BUFFER_RECT);
  if (command == NULL) return NULL;
  command->mem_dst_ = buffer;
  command->mem_dst_->Retain();
  memcpy(command->src_origin_, host_origin, sizeof(size_t) * 3);
  memcpy(command->dst_origin_, buffer_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->src_row_pitch_ = host_row_pitch;
  command->src_slice_pitch_ = host_slice_pitch;
  command->dst_row_pitch_ = buffer_row_pitch;
  command->dst_slice_pitch_ = buffer_slice_pitch;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateFillBuffer(CLContext* context, CLDevice* device,
                            CLCommandQueue* queue, CLMem* buffer,
                            const void* pattern, size_t pattern_size,
                            size_t offset, size_t size) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_FILL_BUFFER);
  if (command == NULL) return NULL;
  command->mem_dst_ = buffer;
  command->mem_dst_->Retain();
  command->pattern_ = malloc(pattern_size);
  memcpy(command->pattern_, pattern, pattern_size);
  command->pattern_size_ = pattern_size;
  command->off_dst_ = offset;
  command->size_ = size;
  return command;
}

CLCommand*
CLCommand::CreateCopyBuffer(CLContext* context, CLDevice* device,
                            CLCommandQueue* queue, CLMem* src_buffer,
                            CLMem* dst_buffer, size_t src_offset,
                            size_t dst_offset, size_t size) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  command->off_src_ = src_offset;
  command->off_dst_ = dst_offset;
  command->size_ = size;
  return command;
}

CLCommand*
CLCommand::CreateCopyBufferRect(CLContext* context, CLDevice* device,
                                CLCommandQueue* queue, CLMem* src_buffer,
                                CLMem* dst_buffer, const size_t* src_origin,
                                const size_t* dst_origin, const size_t* region,
                                size_t src_row_pitch, size_t src_slice_pitch,
                                size_t dst_row_pitch, size_t dst_slice_pitch) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_BUFFER_RECT);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  memcpy(command->src_origin_, src_origin, sizeof(size_t) * 3);
  memcpy(command->dst_origin_, dst_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->src_row_pitch_ = src_row_pitch;
  command->src_slice_pitch_ = src_slice_pitch;
  command->dst_row_pitch_ = dst_row_pitch;
  command->dst_slice_pitch_ = dst_slice_pitch;
  return command;
}

CLCommand*
CLCommand::CreateReadImage(CLContext* context, CLDevice* device,
                           CLCommandQueue* queue, CLMem* image,
                           const size_t* origin, const size_t* region,
                           size_t row_pitch, size_t slice_pitch, void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_READ_IMAGE);
  if (command == NULL) return NULL;
  command->mem_src_ = image;
  command->mem_src_->Retain();
  memcpy(command->src_origin_, origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->dst_row_pitch_ = row_pitch;
  command->dst_slice_pitch_ = slice_pitch;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateWriteImage(CLContext* context, CLDevice* device,
                            CLCommandQueue* queue, CLMem* image,
                            const size_t* origin, const size_t* region,
                            size_t row_pitch, size_t slice_pitch, void* ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_WRITE_IMAGE);
  if (command == NULL) return NULL;
  command->mem_dst_ = image;
  command->mem_dst_->Retain();
  memcpy(command->dst_origin_, origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->src_row_pitch_ = row_pitch;
  command->src_slice_pitch_ = slice_pitch;
  command->ptr_ = ptr;
  return command;
}

CLCommand*
CLCommand::CreateFillImage(CLContext* context, CLDevice* device,
                           CLCommandQueue* queue, CLMem* image,
                           const void* fill_color, const size_t* origin,
                           const size_t* region) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_FILL_IMAGE);
  if (command == NULL) return NULL;
  command->mem_dst_ = image;
  command->mem_dst_->Retain();
  command->ptr_ = (void*)fill_color;
  memcpy(command->dst_origin_, origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  return command;
}

CLCommand*
CLCommand::CreateCopyImage(CLContext* context, CLDevice* device,
                           CLCommandQueue* queue, CLMem* src_image,
                           CLMem* dst_image, const size_t* src_origin,
                           const size_t* dst_origin, const size_t* region) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_IMAGE);
  if (command == NULL) return NULL;
  command->mem_src_ = src_image;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_image;
  command->mem_dst_->Retain();
  memcpy(command->src_origin_, src_origin, sizeof(size_t) * 3);
  memcpy(command->dst_origin_, dst_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  return command;
}

CLCommand*
CLCommand::CreateCopyImageToBuffer(CLContext* context, CLDevice* device,
                                   CLCommandQueue* queue, CLMem* src_image,
                                   CLMem* dst_buffer, const size_t* src_origin,
                                   const size_t* region, size_t dst_offset) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_IMAGE_TO_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = src_image;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  memcpy(command->src_origin_, src_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->off_dst_ = dst_offset;
  return command;
}

CLCommand*
CLCommand::CreateCopyBufferToImage(CLContext* context, CLDevice* device,
                                   CLCommandQueue* queue, CLMem* src_buffer,
                                   CLMem* dst_image, size_t src_offset,
                                   const size_t* dst_origin,
                                   const size_t* region) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_BUFFER_TO_IMAGE);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_image;
  command->mem_dst_->Retain();
  command->off_src_ = src_offset;
  memcpy(command->dst_origin_, dst_origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  return command;
}

CLCommand*
CLCommand::CreateMapBuffer(CLContext* context, CLDevice* device,
                           CLCommandQueue* queue, CLMem* buffer,
                           cl_map_flags map_flags, size_t offset, size_t size,
                           void* mapped_ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_MAP_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = buffer;
  command->mem_src_->Retain();
  command->map_flags_ = map_flags;
  command->off_src_ = offset;
  command->size_ = size;
  command->ptr_ = mapped_ptr;
  return command;
}

CLCommand*
CLCommand::CreateMapImage(CLContext* context, CLDevice* device,
                          CLCommandQueue* queue, CLMem* image,
                          cl_map_flags map_flags, const size_t* origin,
                          const size_t* region, void* mapped_ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_MAP_IMAGE);
  if (command == NULL) return NULL;
  command->mem_src_ = image;
  command->mem_src_->Retain();
  command->map_flags_ = map_flags;
  memcpy(command->src_origin_, origin, sizeof(size_t) * 3);
  memcpy(command->region_, region, sizeof(size_t) * 3);
  command->ptr_ = mapped_ptr;
  return command;
}

CLCommand*
CLCommand::CreateUnmapMemObject(CLContext* context, CLDevice* device,
                                CLCommandQueue* queue, CLMem* mem,
                                void* mapped_ptr) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_UNMAP_MEM_OBJECT);
  if (command == NULL) return NULL;
  command->mem_src_ = mem;
  command->mem_src_->Retain();
  command->ptr_ = mapped_ptr;
  return command;
}

CLCommand*
CLCommand::CreateMigrateMemObjects(CLContext* context, CLDevice* device,
                                   CLCommandQueue* queue,
                                   cl_uint num_mem_objects,
                                   const cl_mem* mem_list,
                                   cl_mem_migration_flags flags) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_MIGRATE_MEM_OBJECTS);
  if (command == NULL) return NULL;
  command->num_mem_objects_ = num_mem_objects;
  command->mem_list_ = (CLMem**)malloc(sizeof(CLMem*) * num_mem_objects);
  for (uint i = 0; i < num_mem_objects; i++) {
    command->mem_list_[i] = mem_list[i]->c_obj;
    command->mem_list_[i]->Retain();
  }
  command->migration_flags_ = flags;
}

CLCommand*
CLCommand::CreateNDRangeKernel(CLContext* context, CLDevice* device,
                               CLCommandQueue* queue, CLKernel* kernel,
                               cl_uint work_dim,
                               const size_t* global_work_offset,
                               const size_t* global_work_size,
                               const size_t* local_work_size) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_NDRANGE_KERNEL);
  if (command == NULL) return NULL;
  command->kernel_ = kernel;
  command->kernel_->Retain();
  command->work_dim_ = work_dim;
  for (cl_uint i = 0; i < work_dim; i++) {
    command->gwo_[i] = (global_work_offset != NULL) ? global_work_offset[i] :
                                                      0;
    command->gws_[i] = global_work_size[i];
    // Modified in soff-runtime
    command->lws_[i] = (local_work_size != NULL) ? local_work_size[i] : 1;
    command->nwg_[i] = command->gws_[i] / command->lws_[i];
  }
  for (cl_uint i = work_dim; i < 3; i++) {
    command->gwo_[i] = 0;
    command->gws_[i] = 1;
    command->lws_[i] = 1;
    command->nwg_[i] = 1;
  }
  command->kernel_args_ = kernel->ExportArgs();
  return command;
}

CLCommand*
CLCommand::CreateNativeKernel(CLContext* context, CLDevice* device,
                              CLCommandQueue* queue, void (*user_func)(void*),
                              void* args, size_t cb_args,
                              cl_uint num_mem_objects, const cl_mem* mem_list,
                              const void** args_mem_loc) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_NATIVE_KERNEL);
  if (command == NULL) return NULL;
  command->user_func_ = user_func;
  if (args != NULL) {
    command->native_args_ = malloc(cb_args);
    memcpy(command->native_args_, args, cb_args);
    command->size_ = cb_args;
  } else {
    command->size_ = 0;
  }
  if (num_mem_objects > 0) {
    command->num_mem_objects_ = num_mem_objects;
    command->mem_list_ = (CLMem**)malloc(sizeof(CLMem*) * num_mem_objects);
    command->mem_offsets_ = (ptrdiff_t*)malloc(sizeof(ptrdiff_t) *
                                               num_mem_objects);
    for (cl_uint i = 0; i < num_mem_objects; i++) {
      command->mem_list_[i] = mem_list[i]->c_obj;
      command->mem_list_[i]->Retain();
      command->mem_offsets_[i] = (size_t)args_mem_loc[i] - (size_t)args;
    }
  }
  return command;
}

CLCommand*
CLCommand::CreateMarker(CLContext* context, CLDevice* device,
                        CLCommandQueue* queue) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_MARKER);
  return command;
}

CLCommand*
CLCommand::CreateBarrier(CLContext* context, CLDevice* device,
                         CLCommandQueue* queue) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_BARRIER);
  return command;
}

CLCommand*
CLCommand::CreateWaitForEvents(CLContext* context, CLDevice* device,
                               CLCommandQueue* queue) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_WAIT_FOR_EVENTS);
  return command;
}

CLCommand*
CLCommand::CreateBuildProgram(CLDevice* device, CLProgram* program,
                              CLProgramSource* source, CLProgramBinary* binary,
                              const char* options) {
  CLCommand* command = new CLCommand(program->context(), device, NULL,
                                     CL_COMMAND_BUILD_PROGRAM);
  if (command == NULL) return NULL;
  command->program_ = program;
  command->program_->Retain();
  command->source_ = source;
  command->binary_ = binary;
  command->options_ = options;
  return command;
}

CLCommand*
CLCommand::CreateCompileProgram(CLDevice* device, CLProgram* program,
                                CLProgramSource* source, const char* options,
                                vector<CLProgramSource*>& headers) {
  CLCommand* command = new CLCommand(program->context(), device, NULL,
                                     CL_COMMAND_COMPILE_PROGRAM);
  if (command == NULL) return NULL;
  command->program_ = program;
  command->program_->Retain();
  command->source_ = source;
  command->options_ = options;
  if (headers.empty()) {
    command->size_ = 0;
  } else {
    command->size_ = headers.size();
    command->headers_ = (CLProgramSource**)malloc(sizeof(CLProgramSource*) *
                                                  headers.size());
    for (size_t i = 0; i < headers.size(); i++)
      command->headers_[i] = headers[i];
  }
  return command;
}

CLCommand*
CLCommand::CreateLinkProgram(CLDevice* device, CLProgram* program,
                             vector<CLProgramBinary*>& binaries,
                             const char* options) {
  CLCommand* command = new CLCommand(program->context(), device, NULL,
                                     CL_COMMAND_LINK_PROGRAM);
  if (command == NULL) return NULL;
  command->program_ = program;
  command->program_->Retain();
  command->size_ = binaries.size();
  command->link_binaries_ =
      (CLProgramBinary**)malloc(sizeof(CLProgramBinary*) * binaries.size());
  for (size_t i = 0; i < binaries.size(); i++)
    command->link_binaries_[i] = binaries[i];
  command->options_ = options;
  return command;
}

CLCommand*
CLCommand::CreateCustom(CLContext* context, CLDevice* device,
                        CLCommandQueue* queue, void (*custom_function)(void*),
                        void* custom_data) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_CUSTOM);
  if (command == NULL) return NULL;
  command->custom_function_ = custom_function;
  command->custom_data_ = custom_data;
  return command;
}

CLCommand*
CLCommand::CreateNop(CLContext* context, CLDevice* device,
                     CLCommandQueue* queue) {
  CLCommand* command = new CLCommand(context, device, queue, CL_COMMAND_NOP);
  return command;
}

CLCommand*
CLCommand::CreateBroadcastBuffer(CLContext* context, CLDevice* device,
                                 CLCommandQueue* queue, CLMem* src_buffer,
                                 CLMem* dst_buffer, size_t src_offset,
                                 size_t dst_offset, size_t cb) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_BROADCAST_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  command->off_src_ = src_offset;
  command->off_dst_ = dst_offset;
  command->size_ = cb;
  return command;
}

CLCommand*
CLCommand::CreateAlltoAllBuffer(CLContext* context, CLDevice* device,
                                CLCommandQueue* queue, CLMem* src_buffer,
                                CLMem* dst_buffer, size_t src_offset,
                                size_t dst_offset, size_t cb) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_ALLTOALL_BUFFER);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  command->off_src_ = src_offset;
  command->off_dst_ = dst_offset;
  command->size_ = cb;
  return command;
}

CLCommand*
CLCommand::CreateLocalFileOpen(CLContext* context, CLDevice* device,
                               CLCommandQueue* queue, CLFile* file) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_LOCAL_FILE_OPEN);
  if (command == NULL) return NULL;
  command->file_dst_ = file;
  command->file_dst_->Retain();
  command->filename_ = file->filename();
  command->file_open_flags_ = file->open_flags();
  return command;
}

CLCommand*
CLCommand::CreateCopyBufferToFile(CLContext* context, CLDevice* device,
                                  CLCommandQueue* queue, CLMem* src_buffer,
                                  CLFile* dst_file, size_t src_offset,
                                  size_t dst_offset, size_t size) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_BUFFER_TO_FILE);
  if (command == NULL) return NULL;
  command->mem_src_ = src_buffer;
  command->mem_src_->Retain();
  command->file_dst_ = dst_file;
  command->file_dst_->Retain();
  command->off_src_ = src_offset;
  command->off_dst_ = dst_offset;
  command->size_ = size;
  return command;
}

CLCommand*
CLCommand::CreateCopyFileToBuffer(CLContext* context, CLDevice* device,
                                  CLCommandQueue* queue, CLFile* src_file,
                                  CLMem* dst_buffer, size_t src_offset,
                                  size_t dst_offset, size_t size) {
  CLCommand* command = new CLCommand(context, device, queue,
                                     CL_COMMAND_COPY_FILE_TO_BUFFER);
  if (command == NULL) return NULL;
  command->file_src_ = src_file;
  command->file_src_->Retain();
  command->mem_dst_ = dst_buffer;
  command->mem_dst_->Retain();
  command->off_src_ = src_offset;
  command->off_dst_ = dst_offset;
  command->size_ = size;
  return command;
}
