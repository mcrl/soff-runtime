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

#ifndef __SNUCL__CALLBACKS_H
#define __SNUCL__CALLBACKS_H

#include <CL/cl.h>
#include "Structs.h"

class ContextErrorNotificationCallback {
 public:
  ContextErrorNotificationCallback(
      void (CL_CALLBACK *pfn_notify)(const char*, const void*, size_t, void*),
      void* user_data) {
    pfn_notify_ = pfn_notify;
    user_data_ = user_data;
  }

  void run(const char* errinfo, const void* private_info, size_t cb) {
    pfn_notify_(errinfo, private_info, cb, user_data_);
  }

 private:
  void (CL_CALLBACK *pfn_notify_)(const char*, const void*, size_t, void*);
  void* user_data_;
};

class ProgramCallback {
 public:
  ProgramCallback(void (CL_CALLBACK *pfn_notify)(cl_program, void*),
                  void* user_data) {
    pfn_notify_ = pfn_notify;
    user_data_ = user_data;
    count_ = 0;
  }

  void set_count(int count) {
    count_ = count;
  }

  bool countdown(cl_program program) {
    int new_count;
    do {
      new_count = count_ - 1;
    } while (!__sync_bool_compare_and_swap(&count_, count_, new_count));
    if (new_count == 0) {
      run(program);
      delete this;
      return true;
    } else {
      return false;
    }
  }

  void run(cl_program program) {
    pfn_notify_(program, user_data_);
  }

 private:
  void (CL_CALLBACK *pfn_notify_)(cl_program, void*);
  void* user_data_;
  int count_;
};

class EventCallback {
 public:
  EventCallback(void (CL_CALLBACK *pfn_notify)(cl_event, cl_int, void*),
                void* user_data, cl_int command_exec_callback_type) {
    pfn_notify_ = pfn_notify;
    user_data_ = user_data;
    command_exec_callback_type_ = command_exec_callback_type;
  }

  bool passed(cl_int status) {
    return (status <= command_exec_callback_type_);
  }

  bool hit(cl_int status) {
    cl_int compare = (status < 0 ? CL_COMPLETE : status);
    return (command_exec_callback_type_ == compare);
  }

  void run_if(cl_event event, cl_int status) {
    cl_int compare = (status < 0 ? CL_COMPLETE : status);
    if (command_exec_callback_type_ == compare)
      pfn_notify_(event, status, user_data_);
  }

  void run(cl_event event, cl_int status) {
    pfn_notify_(event, status, user_data_);
  }

 private:
  void (CL_CALLBACK* pfn_notify_)(cl_event, cl_int, void*);
  void* user_data_;
  cl_int command_exec_callback_type_;
};

class MemObjectDestructorCallback {
 public:
  MemObjectDestructorCallback(void (CL_CALLBACK *pfn_notify)(cl_mem, void*),
                              void* user_data) {
    pfn_notify_ = pfn_notify;
    user_data_ = user_data;
  }

  void run(cl_mem mem) {
    pfn_notify_(mem, user_data_);
  }

 private:
  void (CL_CALLBACK *pfn_notify_)(cl_mem, void*);
  void* user_data_;
};

#endif // __SNUCL__CALLBACKS_H
