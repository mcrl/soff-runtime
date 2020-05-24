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

#ifndef __SNUCL__CL_API_H
#define __SNUCL__CL_API_H

#include <CL/cl.h>
#include <CL/cl_ext_snucl.h>
#include "ICD.h"

#undef CL_API_SUFFIX__VERSION_1_0
#undef CL_EXT_SUFFIX__VERSION_1_0
#undef CL_API_SUFFIX__VERSION_1_1
#undef CL_EXT_SUFFIX__VERSION_1_1
#undef CL_API_SUFFIX__VERSION_1_2
#undef CL_EXT_SUFFIX__VERSION_1_2
#undef CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED
#undef CL_EXT_PREFIX__VERSION_1_0_DEPRECATED    
#undef CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
#undef CL_EXT_PREFIX__VERSION_1_1_DEPRECATED

#define CL_API_SUFFIX__VERSION_1_0
#define CL_EXT_SUFFIX__VERSION_1_0
#define CL_API_SUFFIX__VERSION_1_1
#define CL_EXT_SUFFIX__VERSION_1_1
#define CL_API_SUFFIX__VERSION_1_2
#define CL_EXT_SUFFIX__VERSION_1_2
#define CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED
#define CL_EXT_PREFIX__VERSION_1_0_DEPRECATED    
#define CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
#define CL_EXT_PREFIX__VERSION_1_1_DEPRECATED

#ifdef EXPORT_APIS
#define SNUCL_API_FUNCTION(func) func
#else
#define SNUCL_API_FUNCTION(func) SnuCL_##func
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Platform API */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetPlatformIDs)(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetPlatformInfo)(
    cl_platform_id platform, cl_platform_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Device APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetDeviceIDs)(
    cl_platform_id platform, cl_device_type device_type, cl_uint num_entries,
    cl_device_id* devices, cl_uint* num_devices) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetDeviceInfo)(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCreateSubDevices)(
    cl_device_id in_device, const cl_device_partition_property* properties,
    cl_uint num_devices, cl_device_id* out_devices, cl_uint* num_devices_ret)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainDevice)(cl_device_id device)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseDevice)(cl_device_id device)
    CL_API_SUFFIX__VERSION_1_2;

/* Context APIs  */
extern CL_API_ENTRY cl_context CL_API_CALL
SNUCL_API_FUNCTION(clCreateContext)(
    const cl_context_properties* properties, cl_uint num_devices,
    const cl_device_id* devices,
    void (CL_CALLBACK *pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_context CL_API_CALL
SNUCL_API_FUNCTION(clCreateContextFromType)(
    const cl_context_properties* properties, cl_device_type device_type,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainContext)(cl_context context)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseContext)(cl_context context)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetContextInfo)(
    cl_context context, cl_context_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Command Queue APIs */
extern CL_API_ENTRY cl_command_queue CL_API_CALL
SNUCL_API_FUNCTION(clCreateCommandQueue)(
    cl_context context, cl_device_id device,
    cl_command_queue_properties properties, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainCommandQueue)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseCommandQueue)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetCommandQueueInfo)(
    cl_command_queue command_queue, cl_command_queue_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Memory Object APIs */
extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateBuffer)(
    cl_context context, cl_mem_flags flags, size_t size, void* host_ptr,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateSubBuffer)(
    cl_mem buffer, cl_mem_flags flags,
    cl_buffer_create_type buffer_create_type, const void* buffer_create_info,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, const cl_image_desc* image_desc,
    void* host_ptr, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainMemObject)(cl_mem memobj)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseMemObject)(cl_mem memobj)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetSupportedImageFormats)(
    cl_context context, cl_mem_flags flags, cl_mem_object_type image_type,
    cl_uint num_entries, cl_image_format* image_formats,
    cl_uint* num_image_formats) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetMemObjectInfo)(
    cl_mem memobj, cl_mem_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetImageInfo)(
    cl_mem image, cl_image_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetMemObjectDestructorCallback)(
    cl_mem memobj, void (CL_CALLBACK *pfn_notify)(cl_mem, void*),
    void* user_data) CL_API_SUFFIX__VERSION_1_1;

/* Sampler APIs */
extern CL_API_ENTRY cl_sampler CL_API_CALL
SNUCL_API_FUNCTION(clCreateSampler)(
    cl_context context, cl_bool normalized_coords,
    cl_addressing_mode addressing_mode, cl_filter_mode filter_mode,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainSampler)(cl_sampler sampler)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseSampler)(cl_sampler sampler)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetSamplerInfo)(
    cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Program Object APIs  */
extern CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithSource)(
    cl_context context, cl_uint count, const char** strings,
    const size_t* lengths, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithBinary)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const size_t* lengths, const unsigned char** binaries,
    cl_int* binary_status, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clCreateProgramWithBuiltInKernels)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const char* kernel_names, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainProgram)(cl_program program)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseProgram)(cl_program program)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clBuildProgram)(
    cl_program program, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, void (CL_CALLBACK *pfn_notify)(cl_program, void*),
    void* user_data) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCompileProgram)(
    cl_program program, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, cl_uint num_input_headers,
    const cl_program* input_headers, const char** header_include_names,
    void (CL_CALLBACK *pfn_notify)(cl_program, void*), void* user_data)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_program CL_API_CALL
SNUCL_API_FUNCTION(clLinkProgram)(
    cl_context context, cl_uint num_devices, const cl_device_id* device_list,
    const char* options, cl_uint num_input_programs,
    const cl_program* input_programs,
    void (CL_CALLBACK *pfn_notify)(cl_program, void*), void* user_data,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clUnloadPlatformCompiler)(cl_platform_id platform)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetProgramInfo)(
    cl_program program, cl_program_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetProgramBuildInfo)(
    cl_program program, cl_device_id device, cl_program_build_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Kernel Object APIs */
extern CL_API_ENTRY cl_kernel CL_API_CALL
SNUCL_API_FUNCTION(clCreateKernel)(
    cl_program program, const char* kernel_name, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clCreateKernelsInProgram)(
    cl_program program, cl_uint num_kernels, cl_kernel* kernels,
    cl_uint* num_kernels_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainKernel)(cl_kernel kernel)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseKernel)(cl_kernel kernel)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetKernelArg)(
    cl_kernel kernel, cl_uint arg_index, size_t arg_size,
    const void* arg_value) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelInfo)(
    cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelArgInfo)(
    cl_kernel kernel, cl_uint arg_index, cl_kernel_arg_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetKernelWorkGroupInfo)(
    cl_kernel kernel, cl_device_id device,
    cl_kernel_work_group_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Event Object APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clWaitForEvents)(
    cl_uint num_events, const cl_event* event_list) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetEventInfo)(
    cl_event event, cl_event_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_event CL_API_CALL
SNUCL_API_FUNCTION(clCreateUserEvent)(cl_context context, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetUserEventStatus)(
    cl_event event, cl_int execution_status) CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetEventCallback)(
    cl_event event, cl_int command_exec_callback_type,
    void (CL_CALLBACK *pfn_notify)(cl_event, cl_int, void*), void* user_data)
    CL_API_SUFFIX__VERSION_1_1;

/* Profiling APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetEventProfilingInfo)(
    cl_event event, cl_profiling_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

/* Flush and Finish APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clFlush)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clFinish)(cl_command_queue command_queue)
    CL_API_SUFFIX__VERSION_1_0;

/* Enqueued Commands APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadBufferRect)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
    const size_t* buffer_origin, const size_t* host_origin,
    const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    size_t offset, size_t size, const void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteBufferRect)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
    const size_t* buffer_origin, const size_t* host_origin,
    const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch,
    size_t host_row_pitch, size_t host_slice_pitch, const void* ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueFillBuffer)(
    cl_command_queue command_queue, cl_mem buffer, const void* pattern,
    size_t pattern_size, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBuffer)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    size_t src_offset, size_t dst_offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferRect)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
    const size_t* src_origin, const size_t* dst_origin, const size_t* region,
    size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch,
    size_t dst_slice_pitch, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReadImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
    const size_t* origin, const size_t* region, size_t row_pitch,
    size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWriteImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
    const size_t* origin, const size_t* region, size_t input_row_pitch,
    size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueFillImage)(
    cl_command_queue command_queue, cl_mem image, const void* fill_color,
    const size_t* origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyImage)(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
    const size_t* src_origin, const size_t* dst_origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyImageToBuffer)(
    cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
    const size_t* src_origin, const size_t* region, size_t dst_offset,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferToImage)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
    size_t src_offset, const size_t* dst_origin, const size_t* region,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY void * CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMapBuffer)(
    cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
    cl_map_flags map_flags, size_t offset, size_t size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY void * CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMapImage)(
    cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
    cl_map_flags map_flags, const size_t* origin, const size_t* region,
    size_t* image_row_pitch, size_t* image_slice_pitch,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueUnmapMemObject)(
    cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMigrateMemObjects)(
    cl_command_queue command_queue, cl_uint num_mem_objects,
    const cl_mem* mem_objects, cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueNDRangeKernel)(
    cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t* global_work_offset, const size_t* global_work_size,
    const size_t* local_work_size, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueTask)(
    cl_command_queue command_queue, cl_kernel kernel,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueNativeKernel)(
    cl_command_queue command_queue, void (*user_func)(void*), void* args,
    size_t cb_args, cl_uint num_mem_objects, const cl_mem* mem_list,
    const void** args_mem_loc, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMarkerWithWaitList)(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBarrierWithWaitList)(
    cl_command_queue command_queue, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clSetCommandQueueProperty)(
    cl_command_queue command_queue, cl_command_queue_properties properties,
    cl_bool enable, cl_command_queue_properties* old_properties)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage2D)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, size_t image_width,
    size_t image_height, size_t image_row_pitch, void* host_ptr,
    cl_int* errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateImage3D)(
    cl_context context, cl_mem_flags flags,
    const cl_image_format* image_format, size_t image_width,
    size_t image_height, size_t image_depth, size_t image_row_pitch,
    size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueMarker)(
    cl_command_queue command_queue, cl_event* event)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueWaitForEvents)(
    cl_command_queue command_queue, cl_uint num_events,
    const cl_event* event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBarrier)(cl_command_queue command_queue)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clUnloadCompiler)(void)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

/* SnuCL Extension - Collective Communication APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAlltoAllBuffer)(
    cl_command_queue* cmd_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset,
    size_t* dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueBroadcastBuffer)(
    cl_command_queue* cmd_queue_list, cl_mem src_buffer,
    cl_uint num_dst_buffers, cl_mem* dst_buffer_list, size_t src_offset,
    size_t* dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueScatterBuffer)(
    cl_command_queue* command_queue_list, cl_mem src_buffer,
    cl_uint num_dst_buffers, cl_mem* dst_buffer_list, size_t src_offset,
    size_t* dst_offset_list, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueGatherBuffer)(
    cl_command_queue command_queue, cl_uint num_src_buffers,
    cl_mem* src_buffer_list, cl_mem dst_buffer, size_t* src_offset_list,
    size_t dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAllGatherBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset,
    size_t* dst_offset, size_t cb, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReduceBuffer)(
    cl_command_queue command_queue, cl_uint num_src_buffers,
    cl_mem* src_buffer_list, cl_mem dst_buffer, size_t* src_offset_list,
    size_t dst_offset, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAllReduceBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReduceScatterBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueScanBuffer)(
    cl_command_queue* command_queue_list, cl_uint num_buffers,
    cl_mem* src_buffer_list, cl_mem* dst_buffer_list, size_t* src_offset_list,
    size_t* dst_offset_list, size_t cb, cl_channel_type datatype,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list);

/* SnuCL Extension - File I/O APIs */
extern CL_API_ENTRY cl_file CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueLocalFileOpen)(
    cl_command_queue command_queue, cl_bool blocking_open,
    const char* filename, cl_file_open_flags flags,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event_list, cl_int* errcode_ret);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyBufferToFile)(
    cl_command_queue command_queue, cl_mem src_buffer, cl_file dst_file,
    size_t src_offset, size_t dst_offset, size_t cb,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueCopyFileToBuffer)(
    cl_command_queue command_queue, cl_file src_file, cl_mem dst_buffer,
    size_t src_offset, size_t dst_offset, size_t cb,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clRetainFile)(cl_file file);
    
extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clReleaseFile)(cl_file file);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetFileHandlerInfo)(
    cl_file file, cl_file_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret);

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clIcdGetPlatformIDsKHR)(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);

#ifdef GL_SUPPORT

/* GL Extension API */
extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateFromGLBuffer)(
    cl_context context, cl_mem_flags flags, cl_GLuint bufobj,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateFromGLTexture)(
    cl_context context, cl_mem_flags flags, cl_GLenum target,
    cl_GLint miplevel, cl_GLuint texture, cl_int* errcode_ret)
    CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_mem CL_API_CALL
SNUCL_API_FUNCTION(clCreateFromGLRenderbuffer)(
    cl_context context, cl_mem_flags flags, cl_GLuint renderbuffer,
    cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetGLObjectInfo)(
    cl_mem memobj, cl_gl_object_type* gl_object_type,
    cl_GLuint* gl_object_name) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetGLTextureInfo)(
    cl_mem memobj, cl_gl_texture_info param_name, size_t param_value_size,
    void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueAcquireGLObjects)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clEnqueueReleaseGLObjects)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
SNUCL_API_FUNCTION(clGetGLContextInfoKHR)(
    const cl_context_properties* properties, cl_gl_context_info param_name,
    size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_event CL_API_CALL
SNUCL_API_FUNCTION(clCreateEventFromGLsyncKHR)(
    cl_context context, cl_GLsync sync, cl_int* errcode_ret)
    CL_EXT_SUFFIX__VERSION_1_1;

#endif

extern CL_API_ENTRY void * CL_API_CALL 
SNUCL_API_FUNCTION(clGetExtensionFunctionAddress)(const char* func_name)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY void * CL_API_CALL 
SNUCL_API_FUNCTION(clGetExtensionFunctionAddressForPlatform)(
    cl_platform_id platform, const char* func_name) CL_API_SUFFIX__VERSION_1_2;

#ifndef EXPORT_APIS

/*
 * Always export below two functions
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR(
    cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);

extern CL_API_ENTRY void * CL_API_CALL
clGetExtensionFunctionAddress(const char* func_name)
    CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

extern CL_API_ENTRY void * CL_API_CALL 
clGetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char* func_name) CL_API_SUFFIX__VERSION_1_2;

#endif

#ifdef __cplusplus
}
#endif

#endif // __SNUCL__CL_API_H
