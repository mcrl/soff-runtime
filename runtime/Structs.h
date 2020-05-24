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

#ifndef __SNUCL__STRUCTS_H
#define __SNUCL__STRUCTS_H

#include <CL/cl.h>

struct _cl_icd_dispatch;
class CLPlatform;
class CLDevice;
class CLContext;
class CLCommandQueue;
class CLMem;
class CLSampler;
class CLProgram;
class CLKernel;
class CLEvent;
class CLFile;

struct _cl_platform_id {
  struct _cl_icd_dispatch *dispatch;
  CLPlatform* c_obj;
};

struct _cl_device_id {
  struct _cl_icd_dispatch *dispatch;
  CLDevice* c_obj;
};

struct _cl_context {
  struct _cl_icd_dispatch *dispatch;
  CLContext* c_obj;
};

struct _cl_command_queue {
  struct _cl_icd_dispatch *dispatch;
  CLCommandQueue* c_obj;
};

struct _cl_mem {
  struct _cl_icd_dispatch *dispatch;
  CLMem* c_obj; 
};

struct _cl_sampler {
  struct _cl_icd_dispatch *dispatch;
  CLSampler* c_obj; 
};

struct _cl_program {
  struct _cl_icd_dispatch *dispatch;
  CLProgram* c_obj;
};

struct _cl_kernel {
  struct _cl_icd_dispatch *dispatch;
  CLKernel* c_obj;
};

struct _cl_event {
  struct _cl_icd_dispatch *dispatch;
  CLEvent* c_obj;
};

struct _cl_file {
  struct _cl_icd_dispatch *dispatch;
  CLFile* c_obj;
};

#endif // __SNUCL__STRUCTS_H
