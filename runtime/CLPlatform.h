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

#ifndef __SNUCL__CL_PLATFORM_H
#define __SNUCL__CL_PLATFORM_H

#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

class CLContext;
class CLDevice;
class CLIssuer;
class CLScheduler;
class ContextErrorNotificationCallback;

class CLPlatform: public CLObject<struct _cl_platform_id, CLPlatform> {
 public:
  CLPlatform();
  virtual ~CLPlatform();

  void Init();

  cl_int GetPlatformInfo(cl_platform_info param_name, size_t param_value_size,
                         void* param_value, size_t* param_value_size_ret);
  cl_int GetDeviceIDs(cl_device_type device_type, cl_uint num_entries,
                      cl_device_id* devices, cl_uint* num_devices);
  CLContext* CreateContextFromDevices(
      const cl_context_properties* properties, cl_uint num_devices,
      const cl_device_id* devices, ContextErrorNotificationCallback* callback,
      cl_int* err);
  CLContext* CreateContextFromType(const cl_context_properties* properties,
                                   cl_device_type device_type,
                                   ContextErrorNotificationCallback* callback,
                                   cl_int* err);

  void GetDevices(std::vector<CLDevice*>& devices);
  CLDevice* GetFirstDevice();
  void AddDevice(CLDevice* device);
  void RemoveDevice(CLDevice* device);

  CLScheduler* AllocIdleScheduler();
  void InvokeAllSchedulers();

 private:
  size_t CheckContextProperties(const cl_context_properties* properties,
                                cl_int* err);

  void InitSchedulers(unsigned int num_scheduler, bool busy_waiting);
  void AddIssuer(CLIssuer* issuer);
  void RemoveIssuerOfDevice(CLDevice* device);
  void AddDeviceToFirstIssuer(CLDevice* device);
  void RemoveDeviceFromFirstIssuer(CLDevice* device);

  const char* profile_;
  const char* version_;
  const char* name_;
  const char* vendor_;
  const char* extensions_;
  const char* suffix_;
  cl_device_type default_device_type_;
#if defined(CLUSTER_PLATFORM) || defined(CLUSTER_SVM_PLATFORM)
  bool is_host_;
#endif

  std::vector<CLDevice*> devices_;
  std::vector<CLScheduler*> schedulers_;
  std::vector<CLIssuer*> issuers_;

  pthread_mutex_t mutex_devices_;
  pthread_mutex_t mutex_issuers_;

 public:
  static CLPlatform* GetPlatform();

 private:
  static CLPlatform* singleton_;
};

#endif // __SNUCL__CL_PLATFORM_H
