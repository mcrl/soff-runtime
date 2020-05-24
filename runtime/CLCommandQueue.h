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

#ifndef __SNUCL__CL_COMMAND_QUEUE_H
#define __SNUCL__CL_COMMAND_QUEUE_H

#include <list>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

class CLCommand;
class CLContext;
class CLDevice;
class CLEvent;

class CLCommandQueue: public CLObject<struct _cl_command_queue,
                                      CLCommandQueue> {
 protected:
  CLCommandQueue(CLContext* context, CLDevice* device,
                 cl_command_queue_properties properties);

 public:
  virtual void Cleanup();
  virtual ~CLCommandQueue();

  CLContext* context() const { return context_; }
  CLDevice* device() const { return device_; }

  cl_int GetCommandQueueInfo(cl_command_queue_info param_name,
                             size_t param_value_size, void* param_value,
                             size_t* param_value_size_ret);

  bool IsProfiled() const {
    return (properties_ & CL_QUEUE_PROFILING_ENABLE);
  }

  virtual CLCommand* Peek() = 0;
  virtual void Enqueue(CLCommand* command) = 0;
  virtual void Dequeue(CLCommand* command) = 0;
  void Flush() {}

 protected:
  void InvokeScheduler();

 private:
  CLContext* context_;
  CLDevice* device_;
  cl_command_queue_properties properties_;

 public:
  static CLCommandQueue* CreateCommandQueue(
      CLContext* context, CLDevice* device,
      cl_command_queue_properties properties, cl_int* err);
};

class CLInOrderCommandQueue: public CLCommandQueue {
 public:
  CLInOrderCommandQueue(CLContext* context, CLDevice* device,
                        cl_command_queue_properties properties);
  virtual ~CLInOrderCommandQueue();

  virtual CLCommand* Peek();
  virtual void Enqueue(CLCommand* command);
  virtual void Dequeue(CLCommand* command);

 private:
  LockFreeQueueMS queue_;
  CLEvent* last_event_;
};

class CLOutOfOrderCommandQueue: public CLCommandQueue {
 public:
  CLOutOfOrderCommandQueue(CLContext* context, CLDevice* device,
                           cl_command_queue_properties properties);
  virtual ~CLOutOfOrderCommandQueue();

  virtual CLCommand* Peek();
  virtual void Enqueue(CLCommand* command);
  virtual void Dequeue(CLCommand* command);

 private:
  std::list<CLCommand*> commands_;
  pthread_mutex_t mutex_commands_;
};

#endif // __SNUCL__CL_COMMAND_QUEUE_H
