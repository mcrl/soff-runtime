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

#ifndef __SNUCL__CL_EVENT_H
#define __SNUCL__CL_EVENT_H

#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

class CLCommand;
class CLCommandQueue;
class CLContext;
class EventCallback;

class CLEvent: public CLObject<struct _cl_event, CLEvent> {
 public:
  CLEvent(CLCommandQueue* queue, CLCommand* command);
  CLEvent(CLContext* context, CLCommand* command);
  CLEvent(CLContext* context);
  virtual ~CLEvent();

  CLContext* context() const { return context_; }
  CLCommandQueue* queue() const { return queue_; }

  cl_int GetEventInfo(cl_event_info param_name, size_t param_value_size,
                      void* param_value, size_t* param_value_size_ret);
  cl_int GetEventProfilingInfo(cl_profiling_info param_name,
                               size_t param_value_size, void* param_value,
                               size_t* param_value_size_ret);

  cl_int SetUserEventStatus(cl_int execution_status);

  bool IsComplete() const {
    return (status_ == CL_COMPLETE || status_ < 0);
  }
  bool IsError() const {
    return (status_ < 0);
  }

  void SetStatus(cl_int status);
  cl_int Wait();

  void AddCallback(EventCallback* callback);

 private:
  cl_ulong GetTimestamp();

  CLContext* context_;
  CLCommandQueue* queue_;
  cl_command_type command_type_;
  cl_int status_;

  std::vector<EventCallback*> callbacks_;

  bool profiled_;
  cl_ulong profile_[4];

  pthread_mutex_t mutex_complete_;
  pthread_cond_t cond_complete_;
  pthread_mutex_t mutex_callbacks_;
};

#endif // __SNUCL__CL_EVENT_H
