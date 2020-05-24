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

#include "CLEvent.h"
#include <vector>
#include <pthread.h>
#include <time.h>
#include <CL/cl.h>
#include "Callbacks.h"
#include "CLCommand.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLObject.h"
#include "CLPlatform.h"
#include "IDEProfiler.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

CLEvent::CLEvent(CLCommandQueue* queue, CLCommand* command) {
  context_ = queue->context();
  context_->Retain();

  queue_ = queue;
  queue_->Retain();
  command_type_ = command->type();
  status_ = CL_QUEUED;

  profiled_ = queue->IsProfiled();
  if (profiled_)
    profile_[CL_QUEUED] = GetTimestamp();

  pthread_mutex_init(&mutex_complete_, NULL);
  pthread_cond_init(&cond_complete_, NULL);
  pthread_mutex_init(&mutex_callbacks_, NULL);
}

CLEvent::CLEvent(CLContext* context, CLCommand* command) {
  context_ = context;
  context_->Retain();

  queue_ = NULL;
  command_type_ = command->type();
  status_ = CL_SUBMITTED;

  profiled_ = false;

  pthread_mutex_init(&mutex_complete_, NULL);
  pthread_cond_init(&cond_complete_, NULL);
  pthread_mutex_init(&mutex_callbacks_, NULL);
}

CLEvent::CLEvent(CLContext* context) {
  context_ = context;
  context_->Retain();

  queue_ = NULL;
  command_type_ = CL_COMMAND_USER;
  status_ = CL_SUBMITTED;

  profiled_ = false;

  pthread_mutex_init(&mutex_complete_, NULL);
  pthread_cond_init(&cond_complete_, NULL);
  pthread_mutex_init(&mutex_callbacks_, NULL);
}

CLEvent::~CLEvent() {
  if (queue_) queue_->Release();
  context_->Release();

  for (vector<EventCallback*>::iterator it = callbacks_.begin();
       it != callbacks_.end();
       ++it) {
    delete (*it);
  }

  pthread_mutex_destroy(&mutex_complete_);
  pthread_cond_destroy(&cond_complete_);
  pthread_mutex_destroy(&mutex_callbacks_);
}

cl_int CLEvent::GetEventInfo(cl_event_info param_name, size_t param_value_size,
                             void* param_value, size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO_T(CL_EVENT_COMMAND_QUEUE, cl_command_queue,
                      (queue_ == NULL ? NULL : queue_->st_obj()));
    GET_OBJECT_INFO_T(CL_EVENT_CONTEXT, cl_context, context_->st_obj());
    GET_OBJECT_INFO(CL_EVENT_COMMAND_TYPE, cl_command_type, command_type_);
    GET_OBJECT_INFO_T(CL_EVENT_COMMAND_EXECUTION_STATUS, cl_int, status_);
    GET_OBJECT_INFO_T(CL_EVENT_REFERENCE_COUNT, cl_uint, ref_cnt());
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLEvent::GetEventProfilingInfo(cl_profiling_info param_name,
                                      size_t param_value_size,
                                      void* param_value,
                                      size_t* param_value_size_ret) {
  if (!profiled_) return CL_PROFILING_INFO_NOT_AVAILABLE;
  if (status_ != CL_COMPLETE) return CL_PROFILING_INFO_NOT_AVAILABLE;
  switch (param_name) {
    GET_OBJECT_INFO(CL_PROFILING_COMMAND_QUEUED, cl_ulong,
                    profile_[CL_QUEUED]);
    GET_OBJECT_INFO(CL_PROFILING_COMMAND_SUBMIT, cl_ulong,
                    profile_[CL_SUBMITTED]);
    GET_OBJECT_INFO(CL_PROFILING_COMMAND_START, cl_ulong,
                    profile_[CL_RUNNING]);
    GET_OBJECT_INFO(CL_PROFILING_COMMAND_END, cl_ulong, profile_[CL_COMPLETE]);
    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLEvent::SetUserEventStatus(cl_int execution_status) {
  if (command_type_ != CL_COMMAND_USER) return CL_INVALID_EVENT;
  if (status_ == CL_COMPLETE || status_ < 0)
    return CL_INVALID_OPERATION;
  SetStatus(execution_status);
  return CL_SUCCESS;
}

void CLEvent::SetStatus(cl_int status) {
  if (status == CL_COMPLETE || status < 0) {
    pthread_mutex_lock(&mutex_complete_);
    status_ = status;
    pthread_cond_broadcast(&cond_complete_);
    pthread_mutex_unlock(&mutex_complete_);
  } else {
    status_ = status;
  }

  if (profiled_) {
    profile_[status] = GetTimestamp();
    if (status == CL_COMPLETE || status < 0) {
      IDEProfiler::GetProfiler()->Register(this);
    }
  }

  vector<EventCallback*> target_callbacks;
  target_callbacks.reserve(callbacks_.size());
  pthread_mutex_lock(&mutex_callbacks_);
  for (vector<EventCallback*>::iterator it = callbacks_.begin();
       it != callbacks_.end();
       ++it) {
    if ((*it)->hit(status))
      target_callbacks.push_back(*it);
  }
  pthread_mutex_unlock(&mutex_callbacks_);

  for (vector<EventCallback*>::iterator it = target_callbacks.begin();
       it != target_callbacks.end();
       ++it) {
    (*it)->run(st_obj(), status);
  }

  if (status == CL_COMPLETE || status < 0)
    CLPlatform::GetPlatform()->InvokeAllSchedulers();
}

cl_int CLEvent::Wait() {
  pthread_mutex_lock(&mutex_complete_);
  if (status_ != CL_COMPLETE && status_ > 0)
    pthread_cond_wait(&cond_complete_, &mutex_complete_);
  pthread_mutex_unlock(&mutex_complete_);
  return status_;
}

void CLEvent::AddCallback(EventCallback* callback) {
  pthread_mutex_lock(&mutex_callbacks_);
  bool passed = callback->passed(status_);
  if (!passed)
    callbacks_.push_back(callback);
  pthread_mutex_unlock(&mutex_callbacks_);
  if (passed)
    callback->run(st_obj(), status_);
}

cl_ulong CLEvent::GetTimestamp() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  cl_ulong ret = t.tv_sec;
  ret *= 1000000000;
  ret += t.tv_nsec;
  return ret;
}
