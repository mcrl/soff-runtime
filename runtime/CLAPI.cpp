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

#include "CLAPI.h"
#include <cstring>
#include <CL/cl.h>
#include <CL/cl_ext_snucl.h>
#include "Callbacks.h"
#include "CLCommand.h"
#include "CLCommandQueue.h"
#include "CLContext.h"
#include "CLDevice.h"
#include "CLDispatch.h"
#include "CLEvent.h"
#include "CLFile.h"
#include "CLKernel.h"
#include "CLMem.h"
#include "CLObject.h"
#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLSampler.h"
#include "ICD.h"
#include "Structs.h"

using namespace std;

#define IS_INVALID_PLATFORM(platform) \
  (platform != CLPlatform::GetPlatform()->st_obj())

#define IS_INVALID_DEVICE_TYPE(device_type)                               \
  (device_type != CL_DEVICE_TYPE_ALL &&                                   \
   (!(device_type & (CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU |            \
                     CL_DEVICE_TYPE_ACCELERATOR | CL_DEVICE_TYPE_CUSTOM | \
                     CL_DEVICE_TYPE_DEFAULT))))

#define IS_INVALID_DEVICE(device) (device == NULL)

#define IS_INVALID_SUB_DEVICE(device) \
  (device == NULL || !device->c_obj->IsSubDevice())

#define IS_INVALID_CONTEXT(context) (context == NULL)

#define IS_DEVICE_NOT_ASSOCIATED_WITH_CONTEXT(device, context) \
  (!context->c_obj->IsValidDevice(device->c_obj))

#define IS_DEVICES_NOT_ASSOCIATED_WITH_CONTEXT(device_list, num_devices, context) \
  (!context->c_obj->IsValidDevices(num_devices, device_list))

#define IS_INVALID_COMMAND_QUEUE(command_queue) (command_queue == NULL)

#define IS_INVALID_MEM_OBJECT(memobj) (memobj == NULL)

#define IS_INVALID_BUFFER(buffer) \
  (buffer == NULL || !buffer->c_obj->IsBuffer())

#define IS_SUB_BUFFER(buffer) (buffer->c_obj->IsSubBuffer())

#define IS_INVALID_IMAGE(image) (image == NULL || !image->c_obj->IsImage())

#define IS_INVALID_CHILD_MEM_FLAGS(mem, flags) \
  (!mem->c_obj->IsValidChildFlags(flags))

#define IS_INVALID_BUFFER_CREATE_TYPE(buffer_create_type) \
  (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)

#define IS_INVALID_BUFFER_CREATE_REGION(region, buffer)         \
  (region == NULL ||                                            \
   !buffer->c_obj->IsWithinRange(region->origin, region->size))

#define IS_INVALID_IMAGE_SIZE(image_desc, context) \
  (!context->c_obj->IsSupportedImageSize(image_desc))

#define IS_UNSUPPORTED_IMAGE_FORMAT(flags, image_type, image_format, context) \
  (!context->c_obj->IsSupportedImageFormat(flags, image_type, image_format))

#define IS_IMAGE_NOT_SUPPORTED(context) (!context->c_obj->IsImageSupported())

// See CL/cl.h
#define IS_INVALID_IMAGE_TYPE(image_type)     \
  (image_type < CL_MEM_OBJECT_IMAGE2D ||      \
   image_type > CL_MEM_OBJECT_IMAGE1D_BUFFER)

#define IS_INVALID_SAMPLER(sampler) (sampler == NULL)

#define IS_INVALID_PROGRAM(program) (program == NULL)

#define IS_DEVICE_NOT_ASSOCIATED_WITH_PROGRAM(device, program) \
  (!program->c_obj->IsValidDevice(device->c_obj))

#define IS_DEVICES_NOT_ASSOCIATED_WITH_PROGRAM(device_list, num_devices, program) \
  (!program->c_obj->IsValidDevices(num_devices, device_list))

#define IS_PROGRAM_FROM_SOURCE(program) (program->c_obj->HasSource())

#define IS_PROGRAM_FROM_BINARY(program) (!program->c_obj->HasSource())

#define IS_NO_VALID_BINARY(program, device) \
  (!program->c_obj->HasValidBinary(device->c_obj))

#define IS_COMPILER_NOT_AVAILABLE(program) \
  (!program->c_obj->IsCompilerAvailable())

#define IS_LINKER_NOT_AVAILABLE(device) \
  (!device->c_obj->IsLinkerAvailable())

#define HAS_NO_EXECUTABLE_FOR_PROGRAM(program) \
  (!program->c_obj->HasOneExecutable())

#define HAS_MISSING_EXECUTABLE_FOR_PROGRAM(program) \
  (!program->c_obj->HasAllExecutable())

#define IS_INVALID_KERNEL(kernel) (kernel == NULL)

#define IS_INVALID_ARG_INDEX(arg_index, kernel) \
  (arg_index >= kernel->c_obj->num_args())

static bool IS_INVALID_MEM_FLAGS(cl_mem_flags flags) {
  cl_mem_flags read_write = (flags & CL_MEM_READ_WRITE);
  cl_mem_flags read_only = (flags & CL_MEM_READ_ONLY);
  cl_mem_flags write_only = (flags & CL_MEM_WRITE_ONLY);
  if ((read_write && read_only) || (read_write && write_only) ||
      (read_only && write_only))
    return true;
  cl_mem_flags use_host_ptr = (flags & CL_MEM_USE_HOST_PTR);
  cl_mem_flags alloc_host_ptr = (flags & CL_MEM_ALLOC_HOST_PTR);
  cl_mem_flags copy_host_ptr = (flags & CL_MEM_COPY_HOST_PTR);
  if ((use_host_ptr && alloc_host_ptr) || (use_host_ptr && copy_host_ptr) ||
      (alloc_host_ptr && copy_host_ptr))
    return true;
  cl_mem_flags host_no_access = (flags & CL_MEM_HOST_NO_ACCESS);
  cl_mem_flags host_read_only = (flags & CL_MEM_HOST_READ_ONLY);
  cl_mem_flags host_write_only = (flags & CL_MEM_HOST_WRITE_ONLY);
  if ((host_no_access && host_read_only) ||
      (host_no_access && host_write_only) ||
      (host_read_only && host_write_only))
    return true;
  return false;
}

static bool IS_INVALID_IMAGE_FORMAT(const cl_image_format* image_format) {
  if (image_format == NULL) return true;
  cl_channel_order channel_order = image_format->image_channel_order;
  cl_channel_type channel_data_type = image_format->image_channel_data_type;
  // See CL/cl.h
  if (channel_order < CL_R || channel_order > CL_RGBx)
    return true;
  if (channel_data_type < CL_SNORM_INT8 || channel_data_type > CL_FLOAT)
    return true;
  if ((channel_order == CL_INTENSITY || channel_order == CL_LUMINANCE) &&
      (channel_data_type != CL_UNORM_INT8 &&
       channel_data_type != CL_UNORM_INT16 &&
       channel_data_type != CL_SNORM_INT8 &&
       channel_data_type != CL_SNORM_INT16 &&
       channel_data_type != CL_HALF_FLOAT &&
       channel_data_type != CL_FLOAT))
    return true;
  if ((channel_order == CL_RGB || channel_order == CL_RGBx) &&
      (channel_data_type != CL_UNORM_SHORT_565 &&
       channel_data_type != CL_UNORM_SHORT_555 &&
       channel_data_type != CL_UNORM_INT_101010))
    return true;
  if ((channel_order == CL_ARGB || channel_order == CL_BGRA) &&
      (channel_data_type != CL_UNORM_INT8 &&
       channel_data_type != CL_SNORM_INT8 &&
       channel_data_type != CL_SIGNED_INT8 &&
       channel_data_type != CL_UNSIGNED_INT8 &&
       /*
        * Table 5.6 of the OpenCL 1.2 specification only allows
        * CL_UNORM_INT8, CL_SNORM_INT8, CL_SIGNED_INT8, and CL_UNSIGNED_INT8
        * if the channel layout is CL_BGRA.
        * However, some of the OpenCL 1.2 conformance tests try to create
        * CL_RGBA images with the following channel data types:
        */
       channel_data_type != CL_UNORM_INT16 &&
       channel_data_type != CL_SIGNED_INT16 &&
       channel_data_type != CL_SIGNED_INT32 &&
       channel_data_type != CL_UNSIGNED_INT16 &&
       channel_data_type != CL_UNSIGNED_INT32 &&
       channel_data_type != CL_HALF_FLOAT &&
       channel_data_type != CL_FLOAT))
    return true;
  return false;
}

static bool IS_INVALID_IMAGE_DESCRIPTOR(const cl_image_desc* image_desc) {
  if (image_desc == NULL) return true;
  cl_mem_object_type type = image_desc->image_type;
  if (IS_INVALID_IMAGE_TYPE(type))
    return true;
  if (image_desc->image_width < 1)
    return true;
  if ((type == CL_MEM_OBJECT_IMAGE2D || type == CL_MEM_OBJECT_IMAGE3D ||
       type == CL_MEM_OBJECT_IMAGE2D_ARRAY) && image_desc->image_height < 1)
    return true;
  if (type == CL_MEM_OBJECT_IMAGE3D && image_desc->image_depth < 1)
    return true;
  if ((type == CL_MEM_OBJECT_IMAGE1D_ARRAY ||
       type == CL_MEM_OBJECT_IMAGE2D_ARRAY) &&
      image_desc->image_array_size < 1)
    return true;
  // image_row_pitch and image_slice_pitch are checked in CLMem::CreateImage.
  if (image_desc->num_mip_levels != 0 || image_desc->num_samples != 0)
    return true;
  if (type == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
    if (IS_INVALID_BUFFER(image_desc->buffer))
      return true;
  } else {
    if (image_desc->buffer != NULL)
      return true;
  }
  return false;
}

static bool IS_INVALID_SAMPLER_ARGUMENTS(cl_bool normalized_coords,
                                         cl_addressing_mode addressing_mode,
                                         cl_filter_mode filter_mode) {
  if (normalized_coords != CL_TRUE && normalized_coords != CL_FALSE)
    return true;
  if (addressing_mode != CL_ADDRESS_MIRRORED_REPEAT &&
      addressing_mode != CL_ADDRESS_REPEAT &&
      addressing_mode != CL_ADDRESS_CLAMP_TO_EDGE &&
      addressing_mode != CL_ADDRESS_CLAMP &&
      addressing_mode != CL_ADDRESS_NONE)
    return true;
  if (filter_mode != CL_FILTER_NEAREST && filter_mode != CL_FILTER_LINEAR)
    return true;
  if (normalized_coords == CL_FALSE &&
      (addressing_mode == CL_ADDRESS_MIRRORED_REPEAT ||
       addressing_mode == CL_ADDRESS_REPEAT))
    return true;
  return false;
}

#define SET_ERROR_AND_RETURN(err, ret)   \
  {                                      \
    if (errcode_ret) *errcode_ret = err; \
    return ret;                          \
  }

#ifdef __cplusplus
extern "C" {
#endif

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetPlatformIDs)(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms)
    CL_API_SUFFIX__VERSION_1_0 {
  if ((num_entries == 0 && platforms != NULL) ||
      (num_platforms == NULL && platforms == NULL))
    return CL_INVALID_VALUE;

  if (platforms) platforms[0] = CLPlatform::GetPlatform()->st_obj();
  if (num_platforms) *num_platforms = 1;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL 
SNUCL_API_FUNCTION(clGetPlatformInfo)(
    cl_platform_id platform, cl_platform_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  /* 
   * platform refers to the platform ID returned by clGetPlatformIDs or can be
   * NULL. If platform is NULL, the behavior is implementation-defined.
   */
  if (platform == NULL)
    platform = CLPlatform::GetPlatform()->st_obj();

  if (IS_INVALID_PLATFORM(platform))
    return CL_INVALID_PLATFORM;

  CLPlatform* p = platform->c_obj;
  return p->GetPlatformInfo(param_name, param_value_size, param_value,
                            param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetDeviceIDs)(
    cl_platform_id platform, cl_device_type device_type, cl_uint num_entries,
    cl_device_id* devices, cl_uint* num_devices) CL_API_SUFFIX__VERSION_1_0 {
  /* 
   * platform refers to the platform ID returned by clGetPlatformIDs or can be
   * NULL. If platform is NULL, the behavior is implementation-defined.
   */
  if (platform == NULL)
    platform = CLPlatform::GetPlatform()->st_obj();

  if (IS_INVALID_PLATFORM(platform))
    return CL_INVALID_PLATFORM;
  if (IS_INVALID_DEVICE_TYPE(device_type))
    return CL_INVALID_DEVICE_TYPE;
  if ((num_entries == 0 && devices != NULL) ||
      (num_devices == NULL && devices == NULL))
    return CL_INVALID_VALUE;

  CLPlatform* p = platform->c_obj;
  return p->GetDeviceIDs(device_type, num_entries, devices, num_devices);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetDeviceInfo)(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_DEVICE(device))
    return CL_INVALID_DEVICE;

  CLDevice* d = device->c_obj;
  return d->GetDeviceInfo(param_name, param_value_size, param_value,
                          param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCreateSubDevices)(
    cl_device_id in_device, const cl_device_partition_property* properties,
    cl_uint num_devices, cl_device_id* out_devices, cl_uint* num_devices_ret)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_DEVICE(in_device))
    return CL_INVALID_DEVICE;

  CLDevice* d = in_device->c_obj;
  return d->CreateSubDevices(properties, num_devices, out_devices,
                             num_devices_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainDevice)(cl_device_id device)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_SUB_DEVICE(device))
    return CL_INVALID_DEVICE;

  CLDevice* d = device->c_obj;
  d->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseDevice)(cl_device_id device)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_SUB_DEVICE(device))
    return CL_INVALID_DEVICE;

  CLDevice* d = device->c_obj;
  d->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_context CL_API_CALL
SNUCL_API_FUNCTION(clCreateContext)(
    const cl_context_properties* properties, cl_uint num_devices,
    const cl_device_id* devices,
    void (CL_CALLBACK *pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (devices == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (num_devices == 0)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (pfn_notify == NULL && user_data != NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);

  ContextErrorNotificationCallback* callback = NULL;
  if (pfn_notify != NULL) {
    callback = new ContextErrorNotificationCallback(pfn_notify, user_data);
    if (callback == NULL)
      SET_ERROR_AND_RETURN(CL_OUT_OF_HOST_MEMORY, NULL);
  }

  cl_int err = CL_SUCCESS;
  CLContext* context = CLPlatform::GetPlatform()->CreateContextFromDevices(
      properties, num_devices, devices, callback, &err);
  if (err != CL_SUCCESS) {
    delete callback;
    SET_ERROR_AND_RETURN(err, NULL);
  }
  SET_ERROR_AND_RETURN(CL_SUCCESS, context->st_obj());
}

CL_API_ENTRY cl_context CL_API_CALL
SNUCL_API_FUNCTION(clCreateContextFromType)(
    const cl_context_properties* properties, cl_device_type device_type,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (pfn_notify == NULL && user_data != NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_INVALID_DEVICE_TYPE(device_type))
    SET_ERROR_AND_RETURN(CL_INVALID_DEVICE_TYPE, NULL);

  ContextErrorNotificationCallback* callback = NULL;
  if (pfn_notify != NULL) {
    callback = new ContextErrorNotificationCallback(pfn_notify, user_data);
    if (callback == NULL)
      SET_ERROR_AND_RETURN(CL_OUT_OF_HOST_MEMORY, NULL);
  }

  cl_int err = CL_SUCCESS;
  CLContext* context = CLPlatform::GetPlatform()->CreateContextFromType(
      properties, device_type, callback, &err);
  if (err != CL_SUCCESS) {
    delete callback;
    SET_ERROR_AND_RETURN(err, NULL);
  }
  SET_ERROR_AND_RETURN(CL_SUCCESS, context->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainContext)(cl_context context)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    return CL_INVALID_CONTEXT;

  context->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseContext)(cl_context context)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    return CL_INVALID_CONTEXT;

  context->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetContextInfo)(
    cl_context context, cl_context_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    return CL_INVALID_CONTEXT;

  CLContext* c = context->c_obj;
  return c->GetContextInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_command_queue CL_API_CALL
SNUCL_API_FUNCTION(clCreateCommandQueue)(
    cl_context context, cl_device_id device,
    cl_command_queue_properties properties, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (IS_INVALID_DEVICE(device) ||
      IS_DEVICE_NOT_ASSOCIATED_WITH_CONTEXT(device, context))
    SET_ERROR_AND_RETURN(CL_INVALID_DEVICE, NULL);

  cl_int err = CL_SUCCESS;
  CLCommandQueue* command_queue = CLCommandQueue::CreateCommandQueue(
      context->c_obj, device->c_obj, properties, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, command_queue->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainCommandQueue)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_COMMAND_QUEUE(command_queue))
    return CL_INVALID_COMMAND_QUEUE;

  command_queue->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseCommandQueue)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_COMMAND_QUEUE(command_queue))
    return CL_INVALID_COMMAND_QUEUE;

  command_queue->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetCommandQueueInfo)(
    cl_command_queue command_queue, cl_command_queue_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_COMMAND_QUEUE(command_queue))
    return CL_INVALID_COMMAND_QUEUE;

  CLCommandQueue* q = command_queue->c_obj;
  return q->GetCommandQueueInfo(param_name, param_value_size, param_value,
                                param_value_size_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateBuffer)(
    cl_context context, cl_mem_flags flags, size_t size, void* host_ptr,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (IS_INVALID_MEM_FLAGS(flags))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (size == 0)
    SET_ERROR_AND_RETURN(CL_INVALID_BUFFER_SIZE, NULL);
  if (host_ptr == NULL &&
      (flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR)))
    SET_ERROR_AND_RETURN(CL_INVALID_HOST_PTR, NULL);
  if (host_ptr != NULL &&
      (!(flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))))
    SET_ERROR_AND_RETURN(CL_INVALID_HOST_PTR, NULL);

  cl_int err = CL_SUCCESS;
  CLMem* mem = CLMem::CreateBuffer(context->c_obj, flags, size, host_ptr,
                                   &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, mem->st_obj());
}

CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateSubBuffer)(
    cl_mem buffer, cl_mem_flags flags,
    cl_buffer_create_type buffer_create_type, const void* buffer_create_info,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_1 {
  cl_buffer_region* region = (cl_buffer_region*)buffer_create_info;

  if (IS_INVALID_BUFFER(buffer) || IS_SUB_BUFFER(buffer))
    SET_ERROR_AND_RETURN(CL_INVALID_MEM_OBJECT, NULL);
  if (IS_INVALID_CHILD_MEM_FLAGS(buffer, flags))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_INVALID_BUFFER_CREATE_TYPE(buffer_create_type))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_INVALID_BUFFER_CREATE_REGION(region, buffer))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (region->size == 0)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);

  cl_int err = CL_SUCCESS;
  CLMem* mem = CLMem::CreateSubBuffer(buffer->c_obj, flags, region->origin,
                                      region->size, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, mem->st_obj());
}

CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, const cl_image_desc* image_desc,
    void* host_ptr, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (IS_INVALID_MEM_FLAGS(flags))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_INVALID_IMAGE_FORMAT(image_format))
    SET_ERROR_AND_RETURN(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, NULL);
  if (IS_INVALID_IMAGE_DESCRIPTOR(image_desc))
    SET_ERROR_AND_RETURN(CL_INVALID_IMAGE_DESCRIPTOR, NULL);
  if (IS_INVALID_IMAGE_SIZE(image_desc, context))
    SET_ERROR_AND_RETURN(CL_INVALID_IMAGE_SIZE, NULL);
  if (host_ptr == NULL &&
      (flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR)))
    SET_ERROR_AND_RETURN(CL_INVALID_HOST_PTR, NULL);
  if (host_ptr != NULL &&
      (!(flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))))
    SET_ERROR_AND_RETURN(CL_INVALID_HOST_PTR, NULL);
  if (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
    if (IS_INVALID_CHILD_MEM_FLAGS(image_desc->buffer, flags))
      SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  }
  if (IS_UNSUPPORTED_IMAGE_FORMAT(flags, image_desc->image_type, image_format,
                                  context))
    SET_ERROR_AND_RETURN(CL_IMAGE_FORMAT_NOT_SUPPORTED, NULL);
  if (IS_IMAGE_NOT_SUPPORTED(context))
    SET_ERROR_AND_RETURN(CL_INVALID_OPERATION, NULL);

  cl_int err = CL_SUCCESS;
  CLMem* mem = CLMem::CreateImage(context->c_obj, flags, image_format,
                                  image_desc, host_ptr, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, mem->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainMemObject)(cl_mem memobj)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_MEM_OBJECT(memobj))
    return CL_INVALID_MEM_OBJECT;

  memobj->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseMemObject)(cl_mem memobj)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_MEM_OBJECT(memobj))
    return CL_INVALID_MEM_OBJECT;

  memobj->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetSupportedImageFormats)(
    cl_context context, cl_mem_flags flags, cl_mem_object_type image_type,
    cl_uint num_entries, cl_image_format* image_formats,
    cl_uint* num_image_formats) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    return CL_INVALID_CONTEXT;
  if (IS_INVALID_MEM_FLAGS(flags) || IS_INVALID_IMAGE_TYPE(image_type))
    return CL_INVALID_VALUE;
  if (num_entries == 0 && image_formats != NULL)
    return CL_INVALID_VALUE;

  CLContext* c = context->c_obj;
  return c->GetSupportedImageFormats(flags, image_type, num_entries,
                                     image_formats, num_image_formats);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetMemObjectInfo)(
    cl_mem memobj, cl_mem_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_MEM_OBJECT(memobj))
    return CL_INVALID_MEM_OBJECT;

  CLMem* mem = memobj->c_obj;
  return mem->GetMemObjectInfo(param_name, param_value_size, param_value,
                               param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetImageInfo)(
    cl_mem image, cl_image_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_IMAGE(image))
    return CL_INVALID_MEM_OBJECT;

  CLMem* mem = image->c_obj;
  return mem->GetImageInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetMemObjectDestructorCallback)(
    cl_mem memobj, void (CL_CALLBACK *pfn_notify)(cl_mem, void*),
    void* user_data) CL_API_SUFFIX__VERSION_1_1 {
  if (IS_INVALID_MEM_OBJECT(memobj))
    return CL_INVALID_MEM_OBJECT;
  if (pfn_notify == NULL)
    return CL_INVALID_VALUE;

  MemObjectDestructorCallback* callback =
      new MemObjectDestructorCallback(pfn_notify, user_data);
  if (callback == NULL)
    return CL_OUT_OF_HOST_MEMORY;

  memobj->c_obj->AddDestructorCallback(callback);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_sampler CL_API_CALL
SNUCL_API_FUNCTION(clCreateSampler)(
    cl_context context, cl_bool normalized_coords,
    cl_addressing_mode addressing_mode, cl_filter_mode filter_mode,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (IS_INVALID_SAMPLER_ARGUMENTS(normalized_coords, addressing_mode,
                                   filter_mode))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_IMAGE_NOT_SUPPORTED(context))
    SET_ERROR_AND_RETURN(CL_INVALID_OPERATION, NULL);

  CLSampler* sampler = new CLSampler(context->c_obj, normalized_coords,
                                     addressing_mode, filter_mode);
  if (sampler == NULL)
    SET_ERROR_AND_RETURN(CL_OUT_OF_HOST_MEMORY, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, sampler->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainSampler)(cl_sampler sampler)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_SAMPLER(sampler))
    return CL_INVALID_SAMPLER;

  sampler->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseSampler)(cl_sampler sampler)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_SAMPLER(sampler))
    return CL_INVALID_SAMPLER;

  sampler->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetSamplerInfo)(
    cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_SAMPLER(sampler))
    return CL_INVALID_SAMPLER;

  CLSampler* s = sampler->c_obj;
  return s->GetSamplerInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithSource)(
    cl_context context, cl_uint count, const char** strings,
    const size_t* lengths, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (count == 0 || strings == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  for (cl_uint i = 0; i < count; i++)
    if (strings[i] == NULL)
      SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);

  cl_int err = CL_SUCCESS;
  CLProgram* program = CLProgram::CreateProgramWithSource(
      context->c_obj, count, strings, lengths, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, program->st_obj());
}

CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithBinary)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const size_t* lengths, const unsigned char** binaries,
    cl_int* binary_status, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (num_devices == 0 || device_list == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_DEVICES_NOT_ASSOCIATED_WITH_CONTEXT(device_list, num_devices,
                                             context))
    SET_ERROR_AND_RETURN(CL_INVALID_DEVICE, NULL);
  if (lengths == NULL || binaries == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  for (cl_uint i = 0; i < num_devices; i++)
    if (lengths[i] == 0 || binaries[i] == NULL)
      SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);

  cl_int err = CL_SUCCESS;
  CLProgram* program = CLProgram::CreateProgramWithBinary(
      context->c_obj, num_devices, device_list, lengths, binaries,
      binary_status, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, program->st_obj());
}

CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithBuiltInKernels)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const char* kernel_names, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if (num_devices == 0 || device_list == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (kernel_names == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  for (cl_uint i = 0; i < num_devices; i++)
    if (!device_list[i]->c_obj->IsSupportedBuiltInKernels(kernel_names))
      SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_DEVICES_NOT_ASSOCIATED_WITH_CONTEXT(device_list, num_devices,
                                             context))
    SET_ERROR_AND_RETURN(CL_INVALID_DEVICE, NULL);

  cl_int err = CL_SUCCESS;
  CLProgram* program = CLProgram::CreateProgramWithBuiltInKernels(
      context->c_obj, num_devices, device_list, kernel_names, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, program->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainProgram)(cl_program program)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;

  program->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseProgram)(cl_program program)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;

  program->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clBuildProgram)(
    cl_program program, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, void (CL_CALLBACK *pfn_notify)(cl_program, void*),
    void* user_data) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;
  if ((num_devices > 0 && device_list == NULL) ||
      (num_devices == 0 && device_list != NULL))
    return CL_INVALID_VALUE;
  if (pfn_notify == NULL && user_data != NULL)
    return CL_INVALID_VALUE;
  if (IS_DEVICES_NOT_ASSOCIATED_WITH_PROGRAM(device_list, num_devices,
                                             program))
    return CL_INVALID_DEVICE;
  if (IS_PROGRAM_FROM_BINARY(program)) {
    for (cl_uint i = 0; i < num_devices; i++)
      if (IS_NO_VALID_BINARY(program, device_list[i]))
        return CL_INVALID_BINARY;
  }
  if (IS_PROGRAM_FROM_SOURCE(program) && IS_COMPILER_NOT_AVAILABLE(program))
    return CL_COMPILER_NOT_AVAILABLE;
  if (!IS_PROGRAM_FROM_SOURCE(program) && !IS_PROGRAM_FROM_BINARY(program))
    return CL_INVALID_OPERATION;

  ProgramCallback* callback = NULL;
  if (pfn_notify != NULL) {
    callback = new ProgramCallback(pfn_notify, user_data);
    if (callback == NULL) return CL_OUT_OF_HOST_MEMORY;
  }

  CLProgram* p = program->c_obj;
  return p->BuildProgram(num_devices, device_list, options, callback);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCompileProgram)(
    cl_program program, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, cl_uint num_input_headers,
    const cl_program* input_headers, const char** header_include_names,
    void (CL_CALLBACK *pfn_notify)(cl_program, void*), void* user_data)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;
  if ((num_devices > 0 && device_list == NULL) ||
      (num_devices == 0 && device_list != NULL))
    return CL_INVALID_VALUE;
  if (num_input_headers == 0 &&
      (input_headers != NULL || header_include_names != NULL))
    return CL_INVALID_VALUE;
  if (num_input_headers != 0 &&
      (input_headers == NULL || header_include_names == NULL))
    return CL_INVALID_VALUE;
  if (pfn_notify == NULL && user_data != NULL)
    return CL_INVALID_VALUE;
  if (IS_DEVICES_NOT_ASSOCIATED_WITH_PROGRAM(device_list, num_devices,
                                             program))
    return CL_INVALID_DEVICE;
  if (IS_COMPILER_NOT_AVAILABLE(program))
    return CL_COMPILER_NOT_AVAILABLE;
  if (!IS_PROGRAM_FROM_SOURCE(program))
    return CL_INVALID_OPERATION;

  ProgramCallback* callback = NULL;
  if (pfn_notify != NULL) {
    callback = new ProgramCallback(pfn_notify, user_data);
    if (callback == NULL) return CL_OUT_OF_HOST_MEMORY;
  }

  CLProgram* p = program->c_obj;
  return p->CompileProgram(num_devices, device_list, options,
                           num_input_headers, input_headers,
                           header_include_names, callback);
}

CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clLinkProgram)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, cl_uint num_input_programs,
    const cl_program* input_programs,
    void (CL_CALLBACK *pfn_notify)(cl_program, void*), void* user_data,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_CONTEXT(context))
    SET_ERROR_AND_RETURN(CL_INVALID_CONTEXT, NULL);
  if ((num_devices > 0 && device_list == NULL) ||
      (num_devices == 0 && device_list != NULL))
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (num_input_programs == 0 || input_programs == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  for (cl_uint i = 0; i < num_input_programs; i++)
    if (IS_INVALID_PROGRAM(input_programs[i]))
      SET_ERROR_AND_RETURN(CL_INVALID_PROGRAM, NULL);
  if (pfn_notify == NULL && user_data != NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);
  if (IS_DEVICES_NOT_ASSOCIATED_WITH_CONTEXT(device_list, num_devices,
                                             context))
    SET_ERROR_AND_RETURN(CL_INVALID_DEVICE, NULL);
  for (cl_uint i = 0; i < num_devices; i++)
    if (IS_LINKER_NOT_AVAILABLE(device_list[i]))
      SET_ERROR_AND_RETURN(CL_LINKER_NOT_AVAILABLE, NULL);

  ProgramCallback* callback = NULL;
  if (pfn_notify != NULL) {
    callback = new ProgramCallback(pfn_notify, user_data);
    if (callback == NULL)
      SET_ERROR_AND_RETURN(CL_OUT_OF_HOST_MEMORY, NULL);
  }

  cl_int err = CL_SUCCESS;
  CLProgram* program = CLProgram::CreateProgramWithNothing(
      context->c_obj, num_devices, device_list, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);

  err = program->LinkProgram(options, num_input_programs, input_programs,
                             callback);
  if (err != CL_SUCCESS) {
    delete program;
    SET_ERROR_AND_RETURN(err, NULL);
  }
  SET_ERROR_AND_RETURN(CL_SUCCESS, program->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clUnloadPlatformCompiler)(cl_platform_id platform)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_PLATFORM(platform))
    return CL_INVALID_PLATFORM;
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetProgramInfo)(
    cl_program program, cl_program_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;
  if ((param_name == CL_PROGRAM_NUM_KERNELS ||
       param_name == CL_PROGRAM_KERNEL_NAMES) &&
      HAS_NO_EXECUTABLE_FOR_PROGRAM(program))
    return CL_INVALID_PROGRAM_EXECUTABLE;

  CLProgram* p = program->c_obj;
  return p->GetProgramInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetProgramBuildInfo)(
    cl_program program, cl_device_id device, cl_program_build_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;
  if (IS_INVALID_DEVICE(device) ||
      IS_DEVICE_NOT_ASSOCIATED_WITH_PROGRAM(device, program))
    return CL_INVALID_DEVICE;

  CLProgram* p = program->c_obj;
  return p->GetProgramBuildInfo(device->c_obj, param_name, param_value_size,
                                param_value, param_value_size_ret);
}

CL_API_ENTRY cl_kernel CL_API_CALL
SNUCL_API_FUNCTION(clCreateKernel)(
    cl_program program, const char* kernel_name, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    SET_ERROR_AND_RETURN(CL_INVALID_PROGRAM, NULL);
  if (HAS_NO_EXECUTABLE_FOR_PROGRAM(program))
    SET_ERROR_AND_RETURN(CL_INVALID_PROGRAM_EXECUTABLE, NULL);
  if (kernel_name == NULL)
    SET_ERROR_AND_RETURN(CL_INVALID_VALUE, NULL);

  cl_int err = CL_SUCCESS;
  CLKernel* kernel = program->c_obj->CreateKernel(kernel_name, &err);
  if (err != CL_SUCCESS)
    SET_ERROR_AND_RETURN(err, NULL);
  SET_ERROR_AND_RETURN(CL_SUCCESS, kernel->st_obj());
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCreateKernelsInProgram)(
    cl_program program, cl_uint num_kernels, cl_kernel* kernels,
    cl_uint* num_kernels_ret) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_PROGRAM(program))
    return CL_INVALID_PROGRAM;
  if (HAS_MISSING_EXECUTABLE_FOR_PROGRAM(program))
    return CL_INVALID_PROGRAM_EXECUTABLE;

  CLProgram* p = program->c_obj;
  return p->CreateKernelsInProgram(num_kernels, kernels, num_kernels_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainKernel)(cl_kernel kernel)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;

  kernel->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseKernel)(cl_kernel kernel)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;

  kernel->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetKernelArg)(
    cl_kernel kernel, cl_uint arg_index, size_t arg_size,
    const void* arg_value) CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;
  if (IS_INVALID_ARG_INDEX(arg_index, kernel))
    return CL_INVALID_ARG_INDEX;

  return kernel->c_obj->SetKernelArg(arg_index, arg_size, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelInfo)(
    cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;

  CLKernel* k = kernel->c_obj;
  return k->GetKernelInfo(param_name, param_value_size, param_value,
                          param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelArgInfo)(
    cl_kernel kernel, cl_uint arg_index, cl_kernel_arg_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_2 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;
  if (IS_INVALID_ARG_INDEX(arg_index, kernel))
    return CL_INVALID_ARG_INDEX;

  CLKernel* k = kernel->c_obj;
  return k->GetKernelArgInfo(arg_index, param_name, param_value_size,
                             param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelWorkGroupInfo)(
    cl_kernel kernel, cl_device_id device,
    cl_kernel_work_group_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (IS_INVALID_KERNEL(kernel))
    return CL_INVALID_KERNEL;

  CLKernel* k = kernel->c_obj;
  return k->GetKernelWorkGroupInfo(device->c_obj, param_name, param_value_size,
                                   param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clWaitForEvents)(
    cl_uint num_events, const cl_event* event_list)
    CL_API_SUFFIX__VERSION_1_0 {
  if (num_events == 0 || event_list == NULL)
    return CL_INVALID_VALUE;
  for (cl_uint i = 0; i < num_events; i++)
    if (event_list[i] == NULL)
      return CL_INVALID_EVENT;
  for (cl_uint i = 1; i < num_events; i++)
    if (event_list[0]->c_obj->context() != event_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;

  cl_int ret = CL_SUCCESS;
  for (cl_uint i = 0; i < num_events; i++) {
    if (event_list[i]->c_obj->Wait() < 0)
      ret = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return ret;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetEventInfo)(
    cl_event event, cl_event_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (event == NULL) return CL_INVALID_EVENT;

  CLEvent* e = event->c_obj;
  return e->GetEventInfo(param_name, param_value_size, param_value,
                         param_value_size_ret);
}

CL_API_ENTRY cl_event CL_API_CALL
SNUCL_API_FUNCTION(clCreateUserEvent)(cl_context context, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_1 {
  if (context == NULL) {
    if (errcode_ret) *errcode_ret = CL_INVALID_CONTEXT;
    return NULL;
  }

  CLEvent* event = new CLEvent(context->c_obj);
  if (event == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }
  if (errcode_ret) *errcode_ret = CL_SUCCESS;
  return event->st_obj();
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
  if (event == NULL) return CL_INVALID_EVENT;
  event->c_obj->Retain();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
  if (event == NULL) return CL_INVALID_EVENT;
  event->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetUserEventStatus)(
    cl_event event, cl_int execution_status) CL_API_SUFFIX__VERSION_1_1 {
  if (event == NULL) return CL_INVALID_EVENT;
  if (execution_status != CL_COMPLETE && execution_status >= 0)
    return CL_INVALID_VALUE;
  return event->c_obj->SetUserEventStatus(execution_status);

}
CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetEventCallback)(
    cl_event event, cl_int command_exec_callback_type,
    void (CL_CALLBACK *pfn_notify)(cl_event, cl_int, void*), void* user_data)
    CL_API_SUFFIX__VERSION_1_1 {
  if (event == NULL) return CL_INVALID_EVENT;
  if (pfn_notify == NULL) return CL_INVALID_VALUE;
  if (command_exec_callback_type != CL_SUBMITTED &&
      command_exec_callback_type != CL_RUNNING &&
      command_exec_callback_type != CL_COMPLETE)
    return CL_INVALID_VALUE;

  EventCallback* callback = new EventCallback(pfn_notify, user_data,
                                              command_exec_callback_type);
  if (callback == NULL) return CL_OUT_OF_HOST_MEMORY;

  event->c_obj->AddCallback(callback);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetEventProfilingInfo)(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0 {
  if (event == NULL) return CL_INVALID_EVENT;

  CLEvent* e = event->c_obj;
  return e->GetEventProfilingInfo(param_name, param_value_size, param_value,
                                  param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clFlush)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  command_queue->c_obj->Flush();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clFinish)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateMarker(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  CLEvent* blocking = command->ExportEvent();
  q->Enqueue(command);
  blocking->Wait();
  blocking->Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (buffer == NULL) return CL_INVALID_MEM_OBJECT;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (size == 0 || !b->IsWithinRange(offset, size))
    return CL_INVALID_VALUE;
  if (!b->IsHostReadable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateReadBuffer(
      NULL, NULL, q, b, offset, size, ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_read == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_read == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadBufferRect)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    const size_t* buffer_origin, const size_t* host_origin,
    const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_1 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (buffer == NULL) return CL_INVALID_MEM_OBJECT;
  if (region == NULL) return CL_INVALID_VALUE;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  for (int i = 0; i < 3; i++)
    if (region[i] == 0) return CL_INVALID_VALUE;
  if (buffer_row_pitch != 0 && buffer_row_pitch < region[0])
    return CL_INVALID_VALUE;
  if (buffer_slice_pitch != 0 &&
      (buffer_slice_pitch < region[1] * buffer_row_pitch) &&
      ((buffer_slice_pitch % buffer_row_pitch) != 0))
    return CL_INVALID_VALUE;
  if (host_row_pitch != 0 && host_row_pitch < region[0])
    return CL_INVALID_VALUE;
  if (host_slice_pitch != 0 &&
      (host_slice_pitch < region[1] * host_row_pitch) &&
      ((host_slice_pitch % host_row_pitch) != 0))
    return CL_INVALID_VALUE;
  if (!b->IsHostReadable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateReadBufferRect(
      NULL, NULL, q, b, buffer_origin, host_origin, region, buffer_row_pitch,
      buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_read == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_read == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    size_t offset, size_t size, const void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (buffer == NULL) return CL_INVALID_MEM_OBJECT;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (size == 0 || !b->IsWithinRange(offset, size))
    return CL_INVALID_VALUE;
  if (!b->IsHostWritable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateWriteBuffer(
      NULL, NULL, q, b, offset, size, (void*)ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_write == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_write == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteBufferRect)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    const size_t* buffer_origin, const size_t* host_origin,
    const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, const void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_1 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (buffer == NULL) return CL_INVALID_MEM_OBJECT;
  if (region == NULL) return CL_INVALID_VALUE;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  for (int i = 0; i < 3; i++)
    if (region[i] == 0) return CL_INVALID_VALUE;
  if (buffer_row_pitch != 0 && buffer_row_pitch < region[0])
    return CL_INVALID_VALUE;
  if (buffer_slice_pitch != 0 &&
      (buffer_slice_pitch < region[1] * buffer_row_pitch) &&
      ((buffer_slice_pitch % buffer_row_pitch) != 0))
    return CL_INVALID_VALUE;
  if (host_row_pitch != 0 && host_row_pitch < region[0])
    return CL_INVALID_VALUE;
  if (host_slice_pitch != 0 &&
      (host_slice_pitch < region[1] * host_row_pitch) &&
      ((host_slice_pitch % host_row_pitch) != 0))
    return CL_INVALID_VALUE;
  if (!b->IsHostWritable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateWriteBufferRect(
      NULL, NULL, q, b, buffer_origin, host_origin, region, buffer_row_pitch,
      buffer_slice_pitch, host_row_pitch, host_slice_pitch, (void*)ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_write == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_write == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueFillBuffer)(
    cl_command_queue command_queue, cl_mem buffer, const void* pattern,
    size_t pattern_size, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (buffer == NULL) return CL_INVALID_MEM_OBJECT;
  if (pattern == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (pattern_size == 0 ||
      (pattern_size != 1 && pattern_size != 2 && pattern_size != 4 &&
       pattern_size != 8 && pattern_size != 16 && pattern_size != 32 &&
       pattern_size != 64 && pattern_size != 128))
    return CL_INVALID_VALUE;
  if (!b->IsWithinRange(offset, size))
    return CL_INVALID_VALUE;
  if ((offset % pattern_size != 0) || (size % pattern_size != 0))
    return CL_INVALID_VALUE;

  CLCommand* command = CLCommand::CreateFillBuffer(
      NULL, NULL, q, b, pattern, pattern_size, offset, size);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBuffer)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    size_t src_offset, size_t dst_offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_buffer == NULL || dst_buffer == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* sb = src_buffer->c_obj;
  CLMem* db = dst_buffer->c_obj;
  if (q->context() != sb->context() || q->context() != db->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (size == 0 || !sb->IsWithinRange(src_offset, size) ||
      !db->IsWithinRange(dst_offset, size))
    return CL_INVALID_VALUE;
  if ((sb == db) &&
      ((src_offset <= dst_offset && dst_offset < src_offset + size) ||
       (dst_offset <= src_offset && src_offset < dst_offset + size)))
    return CL_MEM_COPY_OVERLAP;

  CLCommand* command = CLCommand::CreateCopyBuffer(
      NULL, NULL, q, sb, db, src_offset, dst_offset, size);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferRect)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    const size_t* src_origin, const size_t* dst_origin, const size_t* region,
    size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
    size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_1 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_buffer == NULL || dst_buffer == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* sb = src_buffer->c_obj;
  CLMem* db = dst_buffer->c_obj;
  if (q->context() != sb->context() || q->context() != db->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateCopyBufferRect(
      NULL, NULL, q, sb, db, src_origin, dst_origin, region, src_row_pitch,
      src_slice_pitch, dst_row_pitch, dst_slice_pitch);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
    const size_t* origin, const size_t* region, size_t row_pitch,
    size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (image == NULL) return CL_INVALID_MEM_OBJECT;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* img = image->c_obj;
  if (q->context() != img->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (!img->IsHostReadable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateReadImage(
      NULL, NULL, q, img, origin, region, row_pitch, slice_pitch, ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_read == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_read == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
    const size_t* origin, const size_t* region, size_t input_row_pitch,
    size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (image == NULL) return CL_INVALID_MEM_OBJECT;
  if (ptr == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* img = image->c_obj;
  if (q->context() != img->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (!img->IsHostWritable())
    return CL_INVALID_OPERATION;

  CLCommand* command = CLCommand::CreateWriteImage(
      NULL, NULL, q, img, origin, region, input_row_pitch, input_slice_pitch,
      (void*)ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_write == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_write == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0)
      return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueFillImage)(
    cl_command_queue command_queue, cl_mem image, const void* fill_color,
    const size_t* origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (image == NULL) return CL_INVALID_MEM_OBJECT;
  if (fill_color == NULL) return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* img = image->c_obj;
  if (q->context() != img->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateFillImage(
      NULL, NULL, q, img, fill_color, origin, region);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyImage)(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
    const size_t* src_origin, const size_t* dst_origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_image == NULL || dst_image == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* simg = src_image->c_obj;
  CLMem* dimg = dst_image->c_obj;
  if (q->context() != simg->context() || q->context() != dimg->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateCopyImage(
      NULL, NULL, q, simg, dimg, src_origin, dst_origin, region);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyImageToBuffer)(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
    const size_t* src_origin, const size_t* region, size_t dst_offset,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_image == NULL || dst_buffer == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* simg = src_image->c_obj;
  CLMem* db = dst_buffer->c_obj;
  if (q->context() != simg->context() || q->context() != db->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateCopyImageToBuffer(
      NULL, NULL, q, simg, db, src_origin, region, dst_offset);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferToImage)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
    size_t src_offset, const size_t* dst_origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_buffer == NULL || dst_image == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* sb = src_buffer->c_obj;
  CLMem* dimg = dst_image->c_obj;
  if (q->context() != sb->context() || q->context() != dimg->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateCopyBufferToImage(
      NULL, NULL, q, sb, dimg, src_offset, dst_origin, region);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMapBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
    cl_map_flags map_flags, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  cl_int err = CL_SUCCESS;

  if (command_queue == NULL)
    err = CL_INVALID_COMMAND_QUEUE;
  else if (buffer == NULL)
    err = CL_INVALID_MEM_OBJECT;
  else if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
           (num_events_in_wait_list == 0 && event_wait_list != NULL))
    err = CL_INVALID_EVENT_WAIT_LIST;

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* b = buffer->c_obj;
  if (q->context() != b->context())
    err = CL_INVALID_CONTEXT;
  else {
    for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
      if (!event_wait_list[i]) {
        err = CL_INVALID_EVENT_WAIT_LIST;
        break;
      }
      if (q->context() != event_wait_list[i]->c_obj->context()) {
        err = CL_INVALID_CONTEXT;
        break;
      }
    }
  }

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  if (!b->IsHostReadable() && (map_flags & CL_MAP_READ))
    err = CL_INVALID_OPERATION;
  else if (!b->IsHostWritable() &&
           (map_flags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION)))
    err = CL_INVALID_OPERATION;

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  void* mapped_ptr = b->MapAsBuffer(map_flags, offset, size);
  if (mapped_ptr == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  CLCommand* command = CLCommand::CreateMapBuffer(
      NULL, NULL, q, b, map_flags, offset, size, mapped_ptr);
  if (command == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_map == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_map == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0) {
      if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
      return mapped_ptr;
    }
  }

  if (errcode_ret) *errcode_ret = CL_SUCCESS;
  return mapped_ptr;
}

CL_API_ENTRY void * CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMapImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
    cl_map_flags map_flags, const size_t* origin, const size_t* region,
    size_t* image_row_pitch, size_t* image_slice_pitch,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  cl_int err = CL_SUCCESS;

  if (command_queue == NULL)
    err = CL_INVALID_COMMAND_QUEUE;
  else if (image == NULL)
    err = CL_INVALID_MEM_OBJECT;
  else if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
           (num_events_in_wait_list == 0 && event_wait_list != NULL))
    err = CL_INVALID_EVENT_WAIT_LIST;

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* img = image->c_obj;
  if (q->context() != img->context())
    err = CL_INVALID_CONTEXT;
  else {
    for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
      if (!event_wait_list[i]) {
        err = CL_INVALID_EVENT_WAIT_LIST;
        break;
      }
      if (q->context() != event_wait_list[i]->c_obj->context()) {
        err = CL_INVALID_CONTEXT;
        break;
      }
    }
  }

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  if (!img->IsHostReadable() && (map_flags & CL_MAP_READ))
    err = CL_INVALID_OPERATION;
  else if (!img->IsHostWritable() &&
           (map_flags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION)))
    err = CL_INVALID_OPERATION;

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  void* mapped_ptr = img->MapAsImage(map_flags, origin, region,
                                     image_row_pitch, image_slice_pitch);
  if (mapped_ptr == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  CLCommand* command = CLCommand::CreateMapImage(
      NULL, NULL, q, img, map_flags, origin, region, mapped_ptr);
  if (command == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_map == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_map == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0) {
      if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
      return mapped_ptr;
    }
  }

  if (errcode_ret) *errcode_ret = CL_SUCCESS;
  return mapped_ptr;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueUnmapMemObject)(
    cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL)
    return CL_INVALID_COMMAND_QUEUE;
  if (memobj == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* m = memobj->c_obj;
  if (q->context() != m->context())
    return CL_INVALID_CONTEXT;;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateUnmapMemObject(
      NULL, NULL, q, m, mapped_ptr);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMigrateMemObjects)(
    cl_command_queue command_queue, cl_uint num_mem_objects,
    const cl_mem* mem_objects, cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2 {
  if (command_queue == NULL)
    return CL_INVALID_COMMAND_QUEUE;
  if (num_mem_objects == 0 || mem_objects == NULL)
    return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  for (cl_uint i = 0; i < num_mem_objects; i++) {
    if (!mem_objects[i])
      return CL_INVALID_MEM_OBJECT;
    if (q->context() != mem_objects[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateMigrateMemObjects(
      NULL, NULL, q, num_mem_objects, mem_objects, flags);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_INVALID_VALUE;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueNDRangeKernel)(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t* global_work_offset, const size_t* global_work_size,
    const size_t* local_work_size, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL)
    return CL_INVALID_COMMAND_QUEUE;
  if (kernel == NULL)
    return CL_INVALID_KERNEL;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLKernel* k = kernel->c_obj;
  if (q->context() != k->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (!k->IsArgsDirty())
    return CL_INVALID_KERNEL_ARGS;
  if (!k->program()->IsValidDevice(q->device()))
    return CL_INVALID_PROGRAM_EXECUTABLE;
  if (work_dim < 1 || work_dim > 3)
    return CL_INVALID_WORK_DIMENSION;
  if (global_work_size == NULL)
    return CL_INVALID_GLOBAL_WORK_SIZE;

  if (local_work_size != NULL) {
    for (cl_uint i = 0; i < work_dim; i++)
      if (global_work_size[i] % local_work_size[i] > 0)
        return CL_INVALID_WORK_GROUP_SIZE;
  }

  CLCommand* command = CLCommand::CreateNDRangeKernel(
      NULL, NULL, q, k, work_dim, global_work_offset, global_work_size,
      local_work_size);
   if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueTask)(
    cl_command_queue command_queue, cl_kernel kernel,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0 {
  size_t one = 1;
  return SNUCL_API_FUNCTION(clEnqueueNDRangeKernel)(command_queue, kernel, 1,
                                                    NULL, &one, &one,
                                                    num_events_in_wait_list,
                                                    event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueNativeKernel)(
    cl_command_queue command_queue, void (*user_func)(void*), void* args,
    size_t cb_args, cl_uint num_mem_objects, const cl_mem* mem_list,
    const void** args_mem_loc, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0 {
  if (command_queue == NULL)
    return CL_INVALID_COMMAND_QUEUE;
  if (user_func == NULL)
    return CL_INVALID_VALUE;
  if ((args == NULL && (cb_args > 0 || num_mem_objects > 0)) ||
      (args != NULL && cb_args == 0))
    return CL_INVALID_VALUE;
  if ((num_mem_objects > 0 && (mem_list == NULL || args_mem_loc == NULL)) ||
      (num_mem_objects == 0 && (mem_list != NULL || args_mem_loc != NULL)))
    return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  for (cl_uint i = 0; i < num_mem_objects; i++) {
    if (!mem_list[i])
      return CL_INVALID_MEM_OBJECT;
    if (q->context() != mem_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  CLCommand* command = CLCommand::CreateNativeKernel(
      NULL, NULL, q, user_func, args, cb_args, num_mem_objects, mem_list,
      args_mem_loc);
   if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMarkerWithWaitList)(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_2 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateMarker(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBarrierWithWaitList)(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_2 {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateBarrier(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetCommandQueueProperty)(
    cl_command_queue command_queue, cl_command_queue_properties properties,
    cl_bool enable, cl_command_queue_properties* old_properties)
    CL_API_SUFFIX__VERSION_1_0 {
  // not yet implemented: is it deprecated?
  return CL_SUCCESS;
}

CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage2D)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, size_t image_width,
    size_t image_height, size_t image_row_pitch, void* host_ptr,
    cl_int* errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  image_desc.image_width = image_width;
  image_desc.image_height = image_height;
  image_desc.image_depth = 1;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = image_row_pitch;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = NULL;

  return SNUCL_API_FUNCTION(clCreateImage)(context, flags, image_format,
                                           &image_desc, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage3D)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, size_t image_width,
    size_t image_height, size_t image_depth, size_t image_row_pitch,
    size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  image_desc.image_width = image_width;
  image_desc.image_height = image_height;
  image_desc.image_depth = image_depth;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = image_row_pitch;
  image_desc.image_slice_pitch = image_slice_pitch;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = NULL;

  return SNUCL_API_FUNCTION(clCreateImage)(context, flags, image_format,
                                           &image_desc, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMarker)(
    cl_command_queue command_queue, cl_event* event)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (event == NULL) return CL_INVALID_VALUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateMarker(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  *event = command->ExportEvent()->st_obj();
  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWaitForEvents)(
    cl_command_queue command_queue, cl_uint num_events,
    const cl_event* event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (num_events == 0 || event_list == NULL) return CL_INVALID_VALUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateWaitForEvents(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events, event_list);
  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBarrier)(cl_command_queue command_queue)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;

  CLCommandQueue* q = command_queue->c_obj;

  CLCommand* command = CLCommand::CreateBarrier(NULL, NULL, q);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clUnloadCompiler)(void)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAlltoAllBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list) {
  for (cl_uint i = 0; i < num_buffers; i++) {
    CLCommandQueue* q = command_queue_list[i]->c_obj;
    CLMem* sb = src_buffer_list[i]->c_obj;
    CLMem* db = dst_buffer_list[i]->c_obj;
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);

    CLCommand* command = CLCommand::CreateAlltoAllBuffer(
        NULL, NULL, q, sb, db, src_offset, dst_offset, cb);
    if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

    command->SetWaitList(num_events_in_wait_list, event_wait_list);
    if (event_list) event_list[i] = command->ExportEvent()->st_obj();
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBroadcastBuffer)(
    cl_command_queue* command_queue_list, cl_mem src_buffer,
    cl_uint num_dst_buffers, cl_mem* dst_buffer_list, size_t src_offset,
    size_t* dst_offset_list, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list) {
  for (cl_uint i = 0; i < num_dst_buffers; i++) {
    CLCommandQueue* q = command_queue_list[i]->c_obj;
    CLMem* sb = src_buffer->c_obj;
    CLMem* db = dst_buffer_list[i]->c_obj;
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);

    CLCommand* command = CLCommand::CreateBroadcastBuffer(
        NULL, NULL, q, sb, db, src_offset, dst_offset, cb);
    if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

    command->SetWaitList(num_events_in_wait_list, event_wait_list);
    if (event_list) event_list[i] = command->ExportEvent()->st_obj();
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueScatterBuffer)(
    cl_command_queue* command_queue_list, cl_mem src_buffer,
    cl_uint num_dst_buffers, cl_mem* dst_buffer_list, size_t src_offset,
    size_t* dst_offset_list, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list) {
  for (cl_uint i = 0; i < num_dst_buffers; i++) {
    CLCommandQueue* q = command_queue_list[i]->c_obj;
    CLMem* sb = src_buffer->c_obj;
    CLMem* db = dst_buffer_list[i]->c_obj;
    size_t src_scatter_offset = src_offset + (cb * i);
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);

    CLCommand* command = CLCommand::CreateCopyBuffer(
        NULL, NULL, q, sb, db, src_scatter_offset, dst_offset, cb);
    if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

    command->SetWaitList(num_events_in_wait_list, event_wait_list);
    if (event_list) event_list[i] = command->ExportEvent()->st_obj();
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueGatherBuffer)(
    cl_command_queue command_queue, cl_uint num_src_buffers,
    cl_mem* src_buffer_list, cl_mem dst_buffer, size_t* src_offset_list,
    size_t dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list) {
  CLCommandQueue* q = command_queue->c_obj;

  for (cl_uint i = 0; i < num_src_buffers; i++) {
    CLMem* sb = src_buffer_list[i]->c_obj;
    CLMem* db = dst_buffer->c_obj;
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    size_t dst_gather_offset = dst_offset + (cb * i);

    CLCommand* command = CLCommand::CreateCopyBuffer(
        NULL, NULL, q, sb, db, src_offset, dst_gather_offset, cb);
    if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

    command->SetWaitList(num_events_in_wait_list, event_wait_list);
    if (event_list) event_list[i] = command->ExportEvent()->st_obj();
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAllGatherBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset,
    size_t* dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list) {
  cl_int err = CL_SUCCESS;
  for (cl_uint i = 0; i < num_buffers; i++) {
    err |= SNUCL_API_FUNCTION(clEnqueueGatherBuffer)(
        command_queue_list[i], num_buffers, src_buffer_list, dst_buffer_list[i],
        src_offset, (dst_offset != NULL ? dst_offset[i] : 0), cb,
        num_events_in_wait_list, event_wait_list, event_list);
  }
  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReduceBuffer)(
    cl_command_queue command_queue, cl_uint num_src_buffers,
    cl_mem* src_buffer_list, cl_mem dst_buffer, size_t* src_offset_list,
    size_t dst_offset, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) {
  void** bufs = (void**)malloc(sizeof(void*) * num_src_buffers);
  for (cl_uint i = 0; i < num_src_buffers; i++) {
    bufs[i] = malloc(cb);
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
        command_queue, src_buffer_list[i], CL_TRUE, src_offset, cb, bufs[i],
        num_events_in_wait_list, event_wait_list, NULL);
  }

  union {
    double d;
    float f;
    unsigned int u;
    int i;
  } sum;
  size_t sum_size = 1;
  memset(&sum, 0, sizeof(sum));

  switch (datatype) {
    case CL_DOUBLE: {
      for (cl_uint i = 0; i < num_src_buffers; i++) {
        double* arr = (double*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(double); j++) {
          sum.d += arr[j];
        }
      }
      sum_size = sizeof(double);
      break;
    }

    case CL_FLOAT: {
      for (cl_uint i = 0; i < num_src_buffers; i++) {
        float* arr = (float*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(float); j++) {
          sum.f += arr[j];
        }
      }
      sum_size = sizeof(float);
      break;
    }

    case CL_UNSIGNED_INT32: {
      for (cl_uint i = 0; i < num_src_buffers; i++) {
        unsigned int* arr = (unsigned int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(unsigned int); j++) {
          sum.u += arr[j];
        }
      }
      sum_size = sizeof(unsigned int);
      break;
    }

    case CL_SIGNED_INT32: {
      for (cl_uint i = 0; i < num_src_buffers; i++) {
        int* arr = (int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(int); j++) {
          sum.i += arr[j];
        }
      }
      sum_size = sizeof(int);
      break;
    }
  }

  SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
      command_queue, dst_buffer, CL_TRUE, dst_offset, sum_size, &sum,
      0, NULL, event);
  for (cl_uint i = 0; i < num_src_buffers; i++) {
    free(bufs[i]);
  }
  free(bufs);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAllReduceBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list) {
  void** bufs = (void**)malloc(sizeof(void*) * num_buffers);
  for (cl_uint i = 0; i < num_buffers; i++) {
    bufs[i] = malloc(cb);
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
        command_queue_list[i], src_buffer_list[i], CL_TRUE, src_offset, cb,
        bufs[i], num_events_in_wait_list, event_wait_list, NULL);
  }

  union {
    double d;
    float f;
    unsigned int u;
    int i;
  } sum;
  size_t sum_size = 1;
  memset(&sum, 0, sizeof(sum));

  switch (datatype) {
    case CL_DOUBLE: {
      for (cl_uint i = 0; i < num_buffers; i++) {
        double* arr = (double*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(double); j++) {
          sum.d += arr[j];
        }
      }
      sum_size = sizeof(double);
      break;
    }

    case CL_FLOAT: {
      for (cl_uint i = 0; i < num_buffers; i++) {
        float* arr = (float*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(float); j++) {
          sum.f += arr[j];
        }
      }
      sum_size = sizeof(float);
      break;
    }

    case CL_UNSIGNED_INT32: {
      for (cl_uint i = 0; i < num_buffers; i++) {
        unsigned int* arr = (unsigned int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(unsigned int); j++) {
          sum.u += arr[j];
        }
      }
      sum_size = sizeof(unsigned int);
      break;
    }

    case CL_SIGNED_INT32: {
      for (cl_uint i = 0; i < num_buffers; i++) {
        int* arr = (int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(int); j++) {
          sum.i += arr[j];
        }
      }
      sum_size = sizeof(int);
      break;
    }
  }

  for (cl_uint i = 0; i < num_buffers; i++) {
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
        command_queue_list[i], dst_buffer_list[i], CL_TRUE, dst_offset, sum_size,
        &sum, 0, NULL, (event_list ? &event_list[i] : NULL));
  }
  for (cl_uint i = 0; i < num_buffers; i++) {
    free(bufs[i]);
  }
  free(bufs);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReduceScatterBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list) {
  void** bufs = (void**)malloc(sizeof(void*) * num_buffers);
  for (cl_uint i = 0; i < num_buffers; i++) {
    bufs[i] = malloc(cb);
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
        command_queue_list[i], src_buffer_list[i], CL_TRUE, src_offset, cb,
        bufs[i], num_events_in_wait_list, event_wait_list, NULL);
  }

  union {
    double d;
    float f;
    unsigned int u;
    int i;
  } sum[num_buffers];
  size_t sum_size = 1;
  memset(sum, 0, sizeof(sum));

  switch (datatype) {
    case CL_DOUBLE: {
      size_t segment_size = (cb / sizeof(double)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        double* arr = (double*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(double); j++) {
          sum[j / segment_size].d += arr[j];
        }
      }
      sum_size = sizeof(double);
      break;
    }

    case CL_FLOAT: {
      size_t segment_size = (cb / sizeof(float)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        float* arr = (float*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(float); j++) {
          sum[j / segment_size].f += arr[j];
        }
      }
      sum_size = sizeof(float);
      break;
    }

    case CL_UNSIGNED_INT32: {
      size_t segment_size = (cb / sizeof(unsigned int)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        unsigned int* arr = (unsigned int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(unsigned int); j++) {
          sum[j / segment_size].u += arr[j];
        }
      }
      sum_size = sizeof(unsigned int);
      break;
    }

    case CL_SIGNED_INT32: {
      size_t segment_size = (cb / sizeof(int)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        int* arr = (int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(int); j++) {
          sum[j / segment_size].i += arr[j];
        }
      }
      sum_size = sizeof(int);
      break;
    }
  }

  for (cl_uint i = 0; i < num_buffers; i++) {
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
        command_queue_list[i], dst_buffer_list[i], CL_TRUE, dst_offset, sum_size,
        &sum[i], 0, NULL, (event_list ? &event_list[i] : NULL));
  }
  for (cl_uint i = 0; i < num_buffers; i++) {
    free(bufs[i]);
  }
  free(bufs);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueScanBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list) {
  void** bufs = (void**)malloc(sizeof(void*) * num_buffers);
  for (cl_uint i = 0; i < num_buffers; i++) {
    bufs[i] = malloc(cb);
    size_t src_offset = (src_offset_list != NULL ? src_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
        command_queue_list[i], src_buffer_list[i], CL_TRUE, src_offset, cb,
        bufs[i], num_events_in_wait_list, event_wait_list, NULL);
  }

  union {
    double d;
    float f;
    unsigned int u;
    int i;
  } sum[num_buffers];
  size_t sum_size = 1;
  memset(sum, 0, sizeof(sum));

  switch (datatype) {
    case CL_DOUBLE: {
      size_t segment_size = (cb / sizeof(double)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        double* arr = (double*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(double); j++) {
          sum[j / segment_size].d += arr[j];
        }
      }
      for (cl_uint i = 1; i < num_buffers; i++) {
        sum[i].d += sum[i - 1].d;
      }
      sum_size = sizeof(double);
      break;
    }

    case CL_FLOAT: {
      size_t segment_size = (cb / sizeof(float)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        float* arr = (float*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(float); j++) {
          sum[j / segment_size].f += arr[j];
        }
      }
      for (cl_uint i = 1; i < num_buffers; i++) {
        sum[i].f += sum[i - 1].f;
      }
      sum_size = sizeof(float);
      break;
    }

    case CL_UNSIGNED_INT32: {
      size_t segment_size = (cb / sizeof(unsigned int)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        unsigned int* arr = (unsigned int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(unsigned int); j++) {
          sum[j / segment_size].u += arr[j];
        }
      }
      for (cl_uint i = 1; i < num_buffers; i++) {
        sum[i].u += sum[i - 1].u;
      }
      sum_size = sizeof(unsigned int);
      break;
    }

    case CL_SIGNED_INT32: {
      size_t segment_size = (cb / sizeof(int)) / num_buffers;
      for (cl_uint i = 0; i < num_buffers; i++) {
        int* arr = (int*)bufs[i];
        for (size_t j = 0; j < cb / sizeof(int); j++) {
          sum[j / segment_size].i += arr[j];
        }
      }
      for (cl_uint i = 1; i < num_buffers; i++) {
        sum[i].i += sum[i - 1].i;
      }
      sum_size = sizeof(int);
      break;
    }
  }

  for (cl_uint i = 0; i < num_buffers; i++) {
    size_t dst_offset = (dst_offset_list != NULL ? dst_offset_list[i] : 0);
    SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
        command_queue_list[i], dst_buffer_list[i], CL_TRUE, dst_offset, sum_size,
        &sum[i], 0, NULL, (event_list ? &event_list[i] : NULL));
  }
  for (cl_uint i = 0; i < num_buffers; i++) {
    free(bufs[i]);
  }
  free(bufs);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_file CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueLocalFileOpen)(
    cl_command_queue command_queue, cl_bool blocking_open,
    const char* filename, cl_file_open_flags flags,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event, cl_int* errcode_ret) {
  cl_int err = CL_SUCCESS;

  if (command_queue == NULL)
    err = CL_INVALID_COMMAND_QUEUE;
  else if (filename == NULL)
    err = CL_INVALID_VALUE;
  else if (!(flags & (CL_FILE_OPEN_WRITE_ONLY | CL_FILE_OPEN_READ_ONLY |
                      CL_FILE_OPEN_READ_WRITE)))
    err = CL_INVALID_VALUE;
  else if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
           (num_events_in_wait_list == 0 && event_wait_list != NULL))
    err = CL_INVALID_EVENT_WAIT_LIST;

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  CLCommandQueue* q = command_queue->c_obj;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i]) {
      err = CL_INVALID_EVENT_WAIT_LIST;
      break;
    }
    if (q->context() != event_wait_list[i]->c_obj->context()) {
      err = CL_INVALID_CONTEXT;
      break;
    }
  }

  if (err != CL_SUCCESS) {
    if (errcode_ret) *errcode_ret = err;
    return NULL;
  }

  CLFile* file = new CLFile(q->context(), q->device(), filename, flags);
  if (file == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  CLCommand* command = CLCommand::CreateLocalFileOpen(NULL, NULL, q, file);
  if (command == NULL) {
    if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
    return NULL;
  }

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  CLEvent* blocking;
  if (blocking_open == CL_TRUE) blocking = command->ExportEvent();
  q->Enqueue(command);
  if (blocking_open == CL_TRUE) {
    cl_int ret = blocking->Wait();
    blocking->Release();
    if (ret < 0) {
      if (errcode_ret) *errcode_ret = CL_OUT_OF_HOST_MEMORY;
      return file->st_obj();
    }
  }

  if (errcode_ret) *errcode_ret = CL_SUCCESS;
  return file->st_obj();
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferToFile)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_file dst_file,
    size_t src_offset, size_t dst_offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_buffer == NULL)
    return CL_INVALID_MEM_OBJECT;
  if (dst_file == NULL)
    return CL_INVALID_VALUE;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLMem* sb = src_buffer->c_obj;
  CLFile* df = dst_file->c_obj;
  if (q->context() != sb->context() || q->context() != df->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (size == 0 || !sb->IsWithinRange(src_offset, size) ||
      !df->IsWithinRange(dst_offset, size))
    return CL_INVALID_VALUE;

  CLCommand* command = CLCommand::CreateCopyBufferToFile(
      NULL, NULL, q, sb, df, src_offset, dst_offset, size);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyFileToBuffer)(
    cl_command_queue command_queue, cl_file src_file, cl_mem dst_buffer,
    size_t src_offset, size_t dst_offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) {
  if (command_queue == NULL) return CL_INVALID_COMMAND_QUEUE;
  if (src_file == NULL)
    return CL_INVALID_VALUE;
  if (dst_buffer == NULL)
    return CL_INVALID_MEM_OBJECT;
  if ((num_events_in_wait_list > 0 && event_wait_list == NULL) ||
      (num_events_in_wait_list == 0 && event_wait_list != NULL))
    return CL_INVALID_EVENT_WAIT_LIST;

  CLCommandQueue* q = command_queue->c_obj;
  CLFile* sf = src_file->c_obj;
  CLMem* db = dst_buffer->c_obj;
  if (q->context() != sf->context() || q->context() != db->context())
    return CL_INVALID_CONTEXT;
  for (cl_uint i = 0; i < num_events_in_wait_list; i++) {
    if (!event_wait_list[i])
      return CL_INVALID_EVENT_WAIT_LIST;
    if (q->context() != event_wait_list[i]->c_obj->context())
      return CL_INVALID_CONTEXT;
  }

  if (size == 0 || !sf->IsWithinRange(src_offset, size) ||
      !db->IsWithinRange(dst_offset, size))
    return CL_INVALID_VALUE;

  CLCommand* command = CLCommand::CreateCopyFileToBuffer(
      NULL, NULL, q, sf, db, src_offset, dst_offset, size);
  if (command == NULL) return CL_OUT_OF_HOST_MEMORY;

  command->SetWaitList(num_events_in_wait_list, event_wait_list);
  if (event) *event = command->ExportEvent()->st_obj();

  q->Enqueue(command);
  return CL_SUCCESS;
}
    
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseFile)(cl_file file) {
  if (file == NULL)
    return CL_INVALID_VALUE;

  file->c_obj->Release();
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clIcdGetPlatformIDsKHR)(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms) {
  if ((num_entries == 0 && platforms != NULL) ||
      (num_entries > 0 && platforms == NULL))
    return CL_INVALID_VALUE;

  if (num_platforms) *num_platforms = 1;
  if (platforms) platforms[0] = CLPlatform::GetPlatform()->st_obj();
  return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL 
SNUCL_API_FUNCTION(clGetExtensionFunctionAddress)(const char* func_name)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  if (strcmp(func_name, "clIcdGetPlatformIDsKHR") == 0)
    return (void*)SNUCL_API_FUNCTION(clIcdGetPlatformIDsKHR);
  return NULL;
}

CL_API_ENTRY void * CL_API_CALL 
SNUCL_API_FUNCTION(clGetExtensionFunctionAddressForPlatform)(
    cl_platform_id platform, const char* func_name)
    CL_API_SUFFIX__VERSION_1_2 {
  if (platform != CLPlatform::GetPlatform()->st_obj())
    return NULL;
  return SNUCL_API_FUNCTION(clGetExtensionFunctionAddress)(func_name);
}

#ifndef EXPORT_APIS

/*
 * Always export below three functions
 */
CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms) {
  return SNUCL_API_FUNCTION(clIcdGetPlatformIDsKHR)(
      num_entries, platforms, num_platforms);
}

CL_API_ENTRY void * CL_API_CALL 
clGetExtensionFunctionAddress(const char* func_name)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED {
  return SNUCL_API_FUNCTION(clGetExtensionFunctionAddress)(func_name);
}

CL_API_ENTRY void * CL_API_CALL
clGetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char* func_name)
    CL_API_SUFFIX__VERSION_1_2 {
  return SNUCL_API_FUNCTION(clGetExtensionFunctionAddressForPlatform)(
      platform, func_name);
}

#endif

#ifdef __cplusplus
}
#endif
