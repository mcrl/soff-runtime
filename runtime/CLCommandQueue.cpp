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

#include "CLCommandQueue.h"
#include <list>
#include <pthread.h>
#include <CL/cl.h>
#include "CLCommand.h"
#include "CLContext.h"
#include "CLDevice.h"
#include "CLEvent.h"
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

#define COMMAND_QUEUE_SIZE 4096

CLCommandQueue::CLCommandQueue(CLContext *context, CLDevice* device,
                               cl_command_queue_properties properties) {
  context_ = context;
  context_->Retain();
  device_ = device;
  device_->AddCommandQueue(this);
  properties_ = properties;
}

void CLCommandQueue::Cleanup() {
  device_->RemoveCommandQueue(this);
}

CLCommandQueue::~CLCommandQueue() {
  context_->Release();
}

cl_int CLCommandQueue::GetCommandQueueInfo(cl_command_queue_info param_name,
                                           size_t param_value_size,
                                           void* param_value,
                                           size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_QUEUE_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO_T(CL_QUEUE_DEVICE, cl_device_id, device_->st_obj());
    GET_OBJECT_INFO_T(CL_QUEUE_REFERENCE_COUNT, cl_uint, ref_cnt());
    GET_OBJECT_INFO(CL_QUEUE_PROPERTIES, cl_command_queue_properties,
                    properties_);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

void CLCommandQueue::InvokeScheduler() {
  device_->InvokeScheduler();
}

CLCommandQueue* CLCommandQueue::CreateCommandQueue(
    CLContext* context, CLDevice* device,
    cl_command_queue_properties properties, cl_int* err) {
  CLCommandQueue* queue;
  if (properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
    queue = new CLOutOfOrderCommandQueue(context, device, properties);
  else
    queue = new CLInOrderCommandQueue(context, device, properties);
  if (queue == NULL) {
    *err = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  return queue;
}

CLInOrderCommandQueue::CLInOrderCommandQueue(
    CLContext* context, CLDevice* device,
    cl_command_queue_properties properties)
    : CLCommandQueue(context, device, properties),
      queue_(COMMAND_QUEUE_SIZE) {
  last_event_ = NULL;
}

CLInOrderCommandQueue::~CLInOrderCommandQueue() {
  if (last_event_)
    last_event_->Release();
}

CLCommand* CLInOrderCommandQueue::Peek() {
  if (queue_.Size() == 0) return NULL;
  CLCommand* command;
  if (queue_.Peek(&command) && command->IsExecutable())
    return command;
  else
    return NULL;
}

void CLInOrderCommandQueue::Enqueue(CLCommand* command) {
  if (last_event_ != NULL) {
    command->AddWaitEvent(last_event_);
    last_event_->Release();
  }
  last_event_ = command->ExportEvent();
  while (!queue_.Enqueue(command)) {}
  InvokeScheduler();
}

void CLInOrderCommandQueue::Dequeue(CLCommand* command) {
  CLCommand* dequeued_command;
  queue_.Dequeue(&dequeued_command);
#ifdef SNUCL_DEBUG
  if (command != dequeued_command)
    SNUCL_ERROR("%s", "Invalid dequeue request");
#endif // SNUCL_DEBUG
}

CLOutOfOrderCommandQueue::CLOutOfOrderCommandQueue(
    CLContext* context, CLDevice* device,
    cl_command_queue_properties properties)
    : CLCommandQueue(context, device, properties) {
  pthread_mutex_init(&mutex_commands_, NULL);
}

CLOutOfOrderCommandQueue::~CLOutOfOrderCommandQueue() {
  pthread_mutex_destroy(&mutex_commands_);
}

CLCommand* CLOutOfOrderCommandQueue::Peek() {
  if (commands_.empty()) return NULL;

  CLCommand* result = NULL;
  pthread_mutex_lock(&mutex_commands_);
  for (list<CLCommand*>::iterator it = commands_.begin();
       it != commands_.end();
       ++it) {
    CLCommand* command = *it;
    if (!command->IsExecutable()) continue;

    if (command->type() == CL_COMMAND_MARKER ||
        command->type() == CL_COMMAND_BARRIER) {
      if (it == commands_.begin())
        result = command;
    } else {
      result = command;
    }
    break;
  }
  pthread_mutex_unlock(&mutex_commands_);
  return result;
}

void CLOutOfOrderCommandQueue::Enqueue(CLCommand* command) {
  pthread_mutex_lock(&mutex_commands_);
  commands_.push_back(command);
  pthread_mutex_unlock(&mutex_commands_);
  InvokeScheduler();
}

void CLOutOfOrderCommandQueue::Dequeue(CLCommand* command) {
  pthread_mutex_lock(&mutex_commands_);
  commands_.remove(command);
  pthread_mutex_unlock(&mutex_commands_);
}
