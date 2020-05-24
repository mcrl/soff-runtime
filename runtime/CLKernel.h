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

#ifndef __SNUCL__CL_KERNEL_H
#define __SNUCL__CL_KERNEL_H

#include <map>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

class CLContext;
class CLDevice;
class CLKernelInfo;
class CLMem;
class CLProgram;
class CLSampler;

typedef struct _CLKernelArg {
  size_t size;
  char value[256];
  bool local;
  CLMem* mem;
  CLSampler* sampler;
  cl_mem_flags flags;
} CLKernelArg;

class CLKernel: public CLObject<struct _cl_kernel, CLKernel> {
public:
  CLKernel(CLContext* context, CLProgram* program, CLKernelInfo* kernel_info);
  virtual void Cleanup();
  virtual ~CLKernel();

  CLContext* context() const { return context_; }
  CLProgram* program() const { return program_; }
  const char* name() const { return name_; }
  cl_uint num_args() const;
  int snucl_index() const;

  cl_int GetKernelInfo(cl_kernel_info param_name, size_t param_value_size,
                       void* param_value, size_t* param_value_size_ret);
  cl_int GetKernelWorkGroupInfo(CLDevice* device,
                                cl_kernel_work_group_info param_name,
                                size_t param_value_size, void* param_value,
                                size_t* param_value_size_ret);
  cl_int GetKernelArgInfo(cl_uint arg_index, cl_kernel_arg_info param_name,
                          size_t param_value_size, void* param_value,
                          size_t* param_value_size_ret);

  cl_int SetKernelArg(cl_uint arg_index, size_t arg_size,
                      const void* arg_value);

  bool IsArgsDirty() { return args_dirty_; }

  std::map<cl_uint, CLKernelArg*>* ExportArgs();

  bool HasDevSpecific(CLDevice* device);
  void* GetDevSpecific(CLDevice* device);

 private:
  CLContext* context_;
  CLProgram* program_;
  CLKernelInfo* kernel_info_;

  const char* name_;
  std::map<cl_uint, CLKernelArg*> args_;
  bool args_dirty_;

  std::map<CLDevice*, void*> dev_specific_;

  pthread_mutex_t mutex_dev_specific_;
};

#endif // __SNUCL__CL_KERNEL_H
