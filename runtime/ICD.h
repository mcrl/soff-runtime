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

#ifndef __SNUCL__ICD_H
#define __SNUCL__ICD_H

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#include <GL/gl.h>

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

#ifdef __cplusplus
extern "C" {
#endif

// Platform APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetPlatformIDs)(
                 cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetPlatformInfo)(
    cl_platform_id   platform, 
    cl_platform_info param_name,
    size_t           param_value_size, 
    void *           param_value,
    size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Device APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetDeviceIDs)(
    cl_platform_id   platform,
    cl_device_type   device_type, 
    cl_uint          num_entries, 
    cl_device_id *   devices, 
    cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetDeviceInfo)(
    cl_device_id    device,
    cl_device_info  param_name, 
    size_t          param_value_size, 
    void *          param_value,
    size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clCreateSubDevices)(
    cl_device_id     in_device,
    const cl_device_partition_property * partition_properties,
    cl_uint          num_entries,
    cl_device_id *   out_devices,
    cl_uint *        num_devices);

typedef CL_API_ENTRY cl_int (CL_API_CALL * pfn_clRetainDevice)(
    cl_device_id     device) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL * pfn_clReleaseDevice)(
    cl_device_id     device) CL_API_SUFFIX__VERSION_1_2;

// Context APIs  
typedef CL_API_ENTRY cl_context (CL_API_CALL *pfn_clCreateContext)(
    const cl_context_properties * properties,
    cl_uint                 num_devices,
    const cl_device_id *    devices,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *                  user_data,
    cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_context (CL_API_CALL *pfn_clCreateContextFromType)(
    const cl_context_properties * properties,
    cl_device_type          device_type,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void *                  user_data,
    cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainContext)(
    cl_context context) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseContext)(
    cl_context context) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetContextInfo)(
    cl_context         context, 
    cl_context_info    param_name, 
    size_t             param_value_size, 
    void *             param_value, 
    size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Command Queue APIs
typedef CL_API_ENTRY cl_command_queue (CL_API_CALL *pfn_clCreateCommandQueue)(
    cl_context                     context, 
    cl_device_id                   device, 
    cl_command_queue_properties    properties,
    cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainCommandQueue)(
    cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseCommandQueue)(
    cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetCommandQueueInfo)(
    cl_command_queue      command_queue,
    cl_command_queue_info param_name,
    size_t                param_value_size,
    void *                param_value,
    size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Memory Object APIs
typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateBuffer)(
    cl_context   context,
    cl_mem_flags flags,
    size_t       size,
    void *       host_ptr,
    cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateImage)(
    cl_context              context,
    cl_mem_flags            flags,
    const cl_image_format * image_format,
    const cl_image_desc *   image_desc,
    void *                  host_ptr,
    cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainMemObject)(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseMemObject)(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetSupportedImageFormats)(
    cl_context           context,
    cl_mem_flags         flags,
    cl_mem_object_type   image_type,
    cl_uint              num_entries,
    cl_image_format *    image_formats,
    cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0;
                                    
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetMemObjectInfo)(
    cl_mem           memobj,
    cl_mem_info      param_name, 
    size_t           param_value_size,
    void *           param_value,
    size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetImageInfo)(
    cl_mem           image,
    cl_image_info    param_name, 
    size_t           param_value_size,
    void *           param_value,
    size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Sampler APIs
typedef CL_API_ENTRY cl_sampler (CL_API_CALL *pfn_clCreateSampler)(
    cl_context          context,
    cl_bool             normalized_coords, 
    cl_addressing_mode  addressing_mode, 
    cl_filter_mode      filter_mode,
    cl_int *            errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainSampler)(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseSampler)(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetSamplerInfo)(
    cl_sampler         sampler,
    cl_sampler_info    param_name,
    size_t             param_value_size,
    void *             param_value,
    size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;
                            
// Program Object APIs
typedef CL_API_ENTRY cl_program (CL_API_CALL *pfn_clCreateProgramWithSource)(
    cl_context        context,
    cl_uint           count,
    const char **     strings,
    const size_t *    lengths,
    cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program (CL_API_CALL *pfn_clCreateProgramWithBinary)(
    cl_context                     context,
    cl_uint                        num_devices,
    const cl_device_id *           device_list,
    const size_t *                 lengths,
    const unsigned char **         binaries,
    cl_int *                       binary_status,
    cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program (CL_API_CALL *pfn_clCreateProgramWithBuiltInKernels)(
    cl_context            context,
    cl_uint               num_devices,
    const cl_device_id *  device_list,
    const char *          kernel_names,
    cl_int *              errcode_ret) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainProgram)(cl_program program) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseProgram)(cl_program program) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clBuildProgram)(
    cl_program           program,
    cl_uint              num_devices,
    const cl_device_id * device_list,
    const char *         options, 
    void (CL_CALLBACK *pfn_notify)(cl_program program, void * user_data),
    void *               user_data) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clCompileProgram)(
    cl_program           program,
    cl_uint              num_devices,
    const cl_device_id * device_list,
    const char *         options,
    cl_uint              num_input_headers,
    const cl_program *   input_headers,
    const char **        header_include_names,
    void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
    void *               user_data) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_program (CL_API_CALL *pfn_clLinkProgram)(
    cl_context           context,
    cl_uint              num_devices,
    const cl_device_id * device_list,
    const char *         options,
    cl_uint              num_input_programs,
    const cl_program *   input_programs,
    void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
    void *               user_data,
    cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clUnloadPlatformCompiler)(
    cl_platform_id     platform) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetProgramInfo)(
    cl_program         program,
    cl_program_info    param_name,
    size_t             param_value_size,
    void *             param_value,
    size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetProgramBuildInfo)(
    cl_program            program,
    cl_device_id          device,
    cl_program_build_info param_name,
    size_t                param_value_size,
    void *                param_value,
    size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;
                            
// Kernel Object APIs
typedef CL_API_ENTRY cl_kernel (CL_API_CALL *pfn_clCreateKernel)(
    cl_program      program,
    const char *    kernel_name,
    cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clCreateKernelsInProgram)(
    cl_program     program,
    cl_uint        num_kernels,
    cl_kernel *    kernels,
    cl_uint *      num_kernels_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainKernel)(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseKernel)(cl_kernel   kernel) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clSetKernelArg)(
    cl_kernel    kernel,
    cl_uint      arg_index,
    size_t       arg_size,
    const void * arg_value) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetKernelInfo)(
    cl_kernel       kernel,
    cl_kernel_info  param_name,
    size_t          param_value_size,
    void *          param_value,
    size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetKernelArgInfo)(
    cl_kernel       kernel,
    cl_uint         arg_indx,
    cl_kernel_arg_info  param_name,
    size_t          param_value_size,
    void *          param_value,
    size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetKernelWorkGroupInfo)(
    cl_kernel                  kernel,
    cl_device_id               device,
    cl_kernel_work_group_info  param_name,
    size_t                     param_value_size,
    void *                     param_value,
    size_t *                   param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

// Event Object APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clWaitForEvents)(
    cl_uint             num_events,
    const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetEventInfo)(
    cl_event         event,
    cl_event_info    param_name,
    size_t           param_value_size,
    void *           param_value,
    size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;
                            
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clRetainEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clReleaseEvent)(cl_event event) CL_API_SUFFIX__VERSION_1_0;

// Profiling APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetEventProfilingInfo)(
    cl_event            event,
    cl_profiling_info   param_name,
    size_t              param_value_size,
    void *              param_value,
    size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;
                                
// Flush and Finish APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clFlush)(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clFinish)(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0;

// Enqueued Commands APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueReadBuffer)(
    cl_command_queue    command_queue,
    cl_mem              buffer,
    cl_bool             blocking_read,
    size_t              offset,
    size_t              cb, 
    void *              ptr,
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueReadBufferRect)(
    cl_command_queue    command_queue,
    cl_mem              buffer,
    cl_bool             blocking_read,
    const size_t *      buffer_origin,
    const size_t *      host_origin, 
    const size_t *      region,
    size_t              buffer_row_pitch,
    size_t              buffer_slice_pitch,
    size_t              host_row_pitch,
    size_t              host_slice_pitch,
    void *              ptr,
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_1;
                            
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueWriteBuffer)(
    cl_command_queue   command_queue, 
    cl_mem             buffer, 
    cl_bool            blocking_write, 
    size_t             offset, 
    size_t             cb, 
    const void *       ptr, 
    cl_uint            num_events_in_wait_list, 
    const cl_event *   event_wait_list, 
    cl_event *         event) CL_API_SUFFIX__VERSION_1_0;
                            
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueWriteBufferRect)(
    cl_command_queue    command_queue,
    cl_mem              buffer,
    cl_bool             blocking_read,
    const size_t *      buffer_origin,
    const size_t *      host_origin, 
    const size_t *      region,
    size_t              buffer_row_pitch,
    size_t              buffer_slice_pitch,
    size_t              host_row_pitch,
    size_t              host_slice_pitch,    
    const void *        ptr,
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueFillBuffer)(
    cl_command_queue   command_queue,
    cl_mem             buffer,
    const void *       pattern,
    size_t             pattern_size,
    size_t             offset,
    size_t             cb,
    cl_uint            num_events_in_wait_list,
    const cl_event *   event_wait_list,
    cl_event *         event) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueCopyBuffer)(
    cl_command_queue    command_queue, 
    cl_mem              src_buffer,
    cl_mem              dst_buffer, 
    size_t              src_offset,
    size_t              dst_offset,
    size_t              cb, 
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_0;
                            
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueCopyBufferRect)(
    cl_command_queue    command_queue, 
    cl_mem              src_buffer,
    cl_mem              dst_buffer, 
    const size_t *      src_origin,
    const size_t *      dst_origin,
    const size_t *      region,
    size_t              src_row_pitch,
    size_t              src_slice_pitch,
    size_t              dst_row_pitch,
    size_t              dst_slice_pitch,
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueReadImage)(
    cl_command_queue     command_queue,
    cl_mem               image,
    cl_bool              blocking_read, 
    const size_t *       origin,
    const size_t *       region,
    size_t               row_pitch,
    size_t               slice_pitch, 
    void *               ptr,
    cl_uint              num_events_in_wait_list,
    const cl_event *     event_wait_list,
    cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueWriteImage)(
    cl_command_queue    command_queue,
    cl_mem              image,
    cl_bool             blocking_write, 
    const size_t *      origin,
    const size_t *      region,
    size_t              input_row_pitch,
    size_t              input_slice_pitch, 
    const void *        ptr,
    cl_uint             num_events_in_wait_list,
    const cl_event *    event_wait_list,
    cl_event *          event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueFillImage)(
    cl_command_queue   command_queue,
    cl_mem             image,
    const void *       fill_color,
    const size_t       origin[3],
    const size_t       region[3],
    cl_uint            num_events_in_wait_list,
    const cl_event *   event_wait_list,
    cl_event *         event) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueCopyImage)(
    cl_command_queue     command_queue,
    cl_mem               src_image,
    cl_mem               dst_image, 
    const size_t *       src_origin,
    const size_t *       dst_origin,
    const size_t *       region, 
    cl_uint              num_events_in_wait_list,
    const cl_event *     event_wait_list,
    cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueCopyImageToBuffer)(
    cl_command_queue command_queue,
    cl_mem           src_image,
    cl_mem           dst_buffer, 
    const size_t *   src_origin,
    const size_t *   region, 
    size_t           dst_offset,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueCopyBufferToImage)(
    cl_command_queue command_queue,
    cl_mem           src_buffer,
    cl_mem           dst_image, 
    size_t           src_offset,
    const size_t *   dst_origin,
    const size_t *   region, 
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * (CL_API_CALL *pfn_clEnqueueMapBuffer)(
    cl_command_queue command_queue,
    cl_mem           buffer,
    cl_bool          blocking_map, 
    cl_map_flags     map_flags,
    size_t           offset,
    size_t           cb,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event,
    cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * (CL_API_CALL *pfn_clEnqueueMapImage)(
    cl_command_queue  command_queue,
    cl_mem            image, 
    cl_bool           blocking_map, 
    cl_map_flags      map_flags, 
    const size_t *    origin,
    const size_t *    region,
    size_t *          image_row_pitch,
    size_t *          image_slice_pitch,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event,
    cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueUnmapMemObject)(
    cl_command_queue command_queue,
    cl_mem           memobj,
    void *           mapped_ptr,
    cl_uint          num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueMigrateMemObjects)(
    cl_command_queue       command_queue,
    cl_uint                num_mem_objects,
    const cl_mem *         mem_objects,
    cl_mem_migration_flags flags,
    cl_uint                num_events_in_wait_list,
    const cl_event *       event_wait_list,
    cl_event *             event) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueNDRangeKernel)(
    cl_command_queue command_queue,
    cl_kernel        kernel,
    cl_uint          work_dim,
    const size_t *   global_work_offset,
    const size_t *   global_work_size,
    const size_t *   local_work_size,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueTask)(
    cl_command_queue  command_queue,
    cl_kernel         kernel,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueNativeKernel)(
    cl_command_queue  command_queue,
    void (CL_CALLBACK * user_func)(void *),
    void *            args,
    size_t            cb_args, 
    cl_uint           num_mem_objects,
    const cl_mem *    mem_list,
    const void **     args_mem_loc,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueMarkerWithWaitList)(
    cl_command_queue  command_queue,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueBarrierWithWaitList)(
    cl_command_queue  command_queue,
    cl_uint           num_events_in_wait_list,
    const cl_event *  event_wait_list,
    cl_event *        event) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY void * (CL_API_CALL *pfn_clGetExtensionFunctionAddressForPlatform)(
    cl_platform_id platform,
    const char *   function_name) CL_API_SUFFIX__VERSION_1_2;

// Deprecated APIs
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clSetCommandQueueProperty)(
    cl_command_queue              command_queue,
    cl_command_queue_properties   properties, 
    cl_bool                       enable,
    cl_command_queue_properties * old_properties) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateImage2D)(
    cl_context              context,
    cl_mem_flags            flags,
    const cl_image_format * image_format,
    size_t                  image_width,
    size_t                  image_height,
    size_t                  image_row_pitch, 
    void *                  host_ptr,
    cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;
                        
typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateImage3D)(
    cl_context              context,
    cl_mem_flags            flags,
    const cl_image_format * image_format,
    size_t                  image_width, 
    size_t                  image_height,
    size_t                  image_depth, 
    size_t                  image_row_pitch, 
    size_t                  image_slice_pitch, 
    void *                  host_ptr,
    cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clUnloadCompiler)(void) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueMarker)(
    cl_command_queue    command_queue,
    cl_event *          event) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueWaitForEvents)(
    cl_command_queue command_queue,
    cl_uint          num_events,
    const cl_event * event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueBarrier)(cl_command_queue command_queue) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY void * (CL_API_CALL *pfn_clGetExtensionFunctionAddress)(const char *function_name) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

// GL and other APIs
typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateFromGLBuffer)(
    cl_context    context,
    cl_mem_flags  flags,
    GLuint        bufobj,
    int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateFromGLTexture)(
    cl_context      context,
    cl_mem_flags    flags,
    cl_GLenum       target,
    cl_GLint        miplevel,
    cl_GLuint       texture,
    cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateFromGLTexture2D)(
    cl_context      context,
    cl_mem_flags    flags,
    GLenum          target,
    GLint           miplevel,
    GLuint          texture,
    cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateFromGLTexture3D)(
    cl_context      context,
    cl_mem_flags    flags,
    GLenum          target,
    GLint           miplevel,
    GLuint          texture,
    cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateFromGLRenderbuffer)(
    cl_context           context,
    cl_mem_flags         flags,
    GLuint               renderbuffer,
    cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetGLObjectInfo)(
    cl_mem               memobj,
    cl_gl_object_type *  gl_object_type,
    GLuint *             gl_object_name) CL_API_SUFFIX__VERSION_1_0;
                  
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetGLTextureInfo)(
    cl_mem               memobj,
    cl_gl_texture_info   param_name,
    size_t               param_value_size,
    void *               param_value,
    size_t *             param_value_size_ret) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueAcquireGLObjects)(
    cl_command_queue     command_queue,
    cl_uint              num_objects,
    const cl_mem *       mem_objects,
    cl_uint              num_events_in_wait_list,
    const cl_event *     event_wait_list,
    cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clEnqueueReleaseGLObjects)(
    cl_command_queue     command_queue,
    cl_uint              num_objects,
    const cl_mem *       mem_objects,
    cl_uint              num_events_in_wait_list,
    const cl_event *     event_wait_list,
    cl_event *           event) CL_API_SUFFIX__VERSION_1_0;

/* cl_khr_gl_sharing */
typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clGetGLContextInfoKHR)(
    const cl_context_properties *properties,
    cl_gl_context_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret);

/* cl_khr_gl_event */
typedef CL_API_ENTRY cl_event (CL_API_CALL *pfn_clCreateEventFromGLsyncKHR)(
    cl_context context,
    cl_GLsync sync,
    cl_int *errcode_ret);

/* cl_khr_d3d10_sharing */
typedef void *pfn_clGetDeviceIDsFromD3D10KHR;
typedef void *pfn_clCreateFromD3D10BufferKHR;
typedef void *pfn_clCreateFromD3D10Texture2DKHR;
typedef void *pfn_clCreateFromD3D10Texture3DKHR;
typedef void *pfn_clEnqueueAcquireD3D10ObjectsKHR;
typedef void *pfn_clEnqueueReleaseD3D10ObjectsKHR;

/* cl_khr_d3d11_sharing */
typedef void *pfn_clGetDeviceIDsFromD3D11KHR;
typedef void *pfn_clCreateFromD3D11BufferKHR;
typedef void *pfn_clCreateFromD3D11Texture2DKHR;
typedef void *pfn_clCreateFromD3D11Texture3DKHR;
typedef void *pfn_clEnqueueAcquireD3D11ObjectsKHR;
typedef void *pfn_clEnqueueReleaseD3D11ObjectsKHR;

/* cl_khr_dx9_media_sharing */
typedef void *pfn_clCreateFromDX9MediaSurfaceKHR;
typedef void *pfn_clEnqueueAcquireDX9MediaSurfacesKHR;
typedef void *pfn_clEnqueueReleaseDX9MediaSurfacesKHR;
typedef void *pfn_clGetDeviceIDsFromDX9MediaAdapterKHR;

/* OpenCL 1.1 */

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clSetEventCallback)(
    cl_event            /* event */,
    cl_int              /* command_exec_callback_type */,
    void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
    void *              /* user_data */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *pfn_clCreateSubBuffer)(
    cl_mem                   /* buffer */,
    cl_mem_flags             /* flags */,
    cl_buffer_create_type    /* buffer_create_type */,
    const void *             /* buffer_create_info */,
    cl_int *                 /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clSetMemObjectDestructorCallback)(
    cl_mem /* memobj */, 
    void (CL_CALLBACK * /*pfn_notify*/)( cl_mem /* memobj */, void* /*user_data*/), 
    void * /*user_data */ ) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_event (CL_API_CALL *pfn_clCreateUserEvent)(
    cl_context    /* context */,
    cl_int *      /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clSetUserEventStatus)(
    cl_event   /* event */,
    cl_int     /* execution_status */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clCreateSubDevicesEXT)(
    cl_device_id     in_device,
    const cl_device_partition_property_ext * partition_properties,
    cl_uint          num_entries,
    cl_device_id *   out_devices,
    cl_uint *        num_devices);

typedef CL_API_ENTRY cl_int (CL_API_CALL * pfn_clRetainDeviceEXT)(
    cl_device_id     device) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL * pfn_clReleaseDeviceEXT)(
    cl_device_id     device) CL_API_SUFFIX__VERSION_1_0;

struct _cl_icd_dispatch {
  pfn_clGetPlatformIDs                         clGetPlatformIDs;
  pfn_clGetPlatformInfo                        clGetPlatformInfo;
  pfn_clGetDeviceIDs                           clGetDeviceIDs;
  pfn_clGetDeviceInfo                          clGetDeviceInfo;
  pfn_clCreateContext                          clCreateContext;
  pfn_clCreateContextFromType                  clCreateContextFromType;
  pfn_clRetainContext                          clRetainContext;
  pfn_clReleaseContext                         clReleaseContext;
  pfn_clGetContextInfo                         clGetContextInfo;
  pfn_clCreateCommandQueue                     clCreateCommandQueue;
  pfn_clRetainCommandQueue                     clRetainCommandQueue;
  pfn_clReleaseCommandQueue                    clReleaseCommandQueue;
  pfn_clGetCommandQueueInfo                    clGetCommandQueueInfo;
  pfn_clSetCommandQueueProperty                clSetCommandQueueProperty;
  pfn_clCreateBuffer                           clCreateBuffer;
  pfn_clCreateImage2D                          clCreateImage2D;
  pfn_clCreateImage3D                          clCreateImage3D;
  pfn_clRetainMemObject                        clRetainMemObject;
  pfn_clReleaseMemObject                       clReleaseMemObject;
  pfn_clGetSupportedImageFormats               clGetSupportedImageFormats;
  pfn_clGetMemObjectInfo                       clGetMemObjectInfo;
  pfn_clGetImageInfo                           clGetImageInfo;
  pfn_clCreateSampler                          clCreateSampler;
  pfn_clRetainSampler                          clRetainSampler;
  pfn_clReleaseSampler                         clReleaseSampler;
  pfn_clGetSamplerInfo                         clGetSamplerInfo;
  pfn_clCreateProgramWithSource                clCreateProgramWithSource;
  pfn_clCreateProgramWithBinary                clCreateProgramWithBinary;
  pfn_clRetainProgram                          clRetainProgram;
  pfn_clReleaseProgram                         clReleaseProgram;
  pfn_clBuildProgram                           clBuildProgram;
  pfn_clUnloadCompiler                         clUnloadCompiler;
  pfn_clGetProgramInfo                         clGetProgramInfo;
  pfn_clGetProgramBuildInfo                    clGetProgramBuildInfo;
  pfn_clCreateKernel                           clCreateKernel;
  pfn_clCreateKernelsInProgram                 clCreateKernelsInProgram;
  pfn_clRetainKernel                           clRetainKernel;
  pfn_clReleaseKernel                          clReleaseKernel;
  pfn_clSetKernelArg                           clSetKernelArg;
  pfn_clGetKernelInfo                          clGetKernelInfo;
  pfn_clGetKernelWorkGroupInfo                 clGetKernelWorkGroupInfo;
  pfn_clWaitForEvents                          clWaitForEvents;
  pfn_clGetEventInfo                           clGetEventInfo;
  pfn_clRetainEvent                            clRetainEvent;
  pfn_clReleaseEvent                           clReleaseEvent;
  pfn_clGetEventProfilingInfo                  clGetEventProfilingInfo;
  pfn_clFlush                                  clFlush;
  pfn_clFinish                                 clFinish;
  pfn_clEnqueueReadBuffer                      clEnqueueReadBuffer;
  pfn_clEnqueueWriteBuffer                     clEnqueueWriteBuffer;
  pfn_clEnqueueCopyBuffer                      clEnqueueCopyBuffer;
  pfn_clEnqueueReadImage                       clEnqueueReadImage;
  pfn_clEnqueueWriteImage                      clEnqueueWriteImage;
  pfn_clEnqueueCopyImage                       clEnqueueCopyImage;
  pfn_clEnqueueCopyImageToBuffer               clEnqueueCopyImageToBuffer;
  pfn_clEnqueueCopyBufferToImage               clEnqueueCopyBufferToImage;
  pfn_clEnqueueMapBuffer                       clEnqueueMapBuffer;
  pfn_clEnqueueMapImage                        clEnqueueMapImage;
  pfn_clEnqueueUnmapMemObject                  clEnqueueUnmapMemObject;
  pfn_clEnqueueNDRangeKernel                   clEnqueueNDRangeKernel;
  pfn_clEnqueueTask                            clEnqueueTask;
  pfn_clEnqueueNativeKernel                    clEnqueueNativeKernel;
  pfn_clEnqueueMarker                          clEnqueueMarker;
  pfn_clEnqueueWaitForEvents                   clEnqueueWaitForEvents;
  pfn_clEnqueueBarrier                         clEnqueueBarrier;
  pfn_clGetExtensionFunctionAddress            clGetExtensionFunctionAddress;
  pfn_clCreateFromGLBuffer                     clCreateFromGLBuffer;
  pfn_clCreateFromGLTexture2D                  clCreateFromGLTexture2D;
  pfn_clCreateFromGLTexture3D                  clCreateFromGLTexture3D;
  pfn_clCreateFromGLRenderbuffer               clCreateFromGLRenderbuffer;
  pfn_clGetGLObjectInfo                        clGetGLObjectInfo;
  pfn_clGetGLTextureInfo                       clGetGLTextureInfo;
  pfn_clEnqueueAcquireGLObjects                clEnqueueAcquireGLObjects;
  pfn_clEnqueueReleaseGLObjects                clEnqueueReleaseGLObjects;
  pfn_clGetGLContextInfoKHR                    clGetGLContextInfoKHR;

  pfn_clGetDeviceIDsFromD3D10KHR               clGetDeviceIDsFromD3D10KHR;
  pfn_clCreateFromD3D10BufferKHR               clCreateFromD3D10BufferKHR;
  pfn_clCreateFromD3D10Texture2DKHR            clCreateFromD3D10Texture2DKHR;
  pfn_clCreateFromD3D10Texture3DKHR            clCreateFromD3D10Texture3DKHR;
  pfn_clEnqueueAcquireD3D10ObjectsKHR          clEnqueueAcquireD3D10ObjectsKHR;
  pfn_clEnqueueReleaseD3D10ObjectsKHR          clEnqueueReleaseD3D10ObjectsKHR;

  pfn_clSetEventCallback                       clSetEventCallback;
  pfn_clCreateSubBuffer                        clCreateSubBuffer;
  pfn_clSetMemObjectDestructorCallback         clSetMemObjectDestructorCallback;
  pfn_clCreateUserEvent                        clCreateUserEvent;
  pfn_clSetUserEventStatus                     clSetUserEventStatus;
  pfn_clEnqueueReadBufferRect                  clEnqueueReadBufferRect;
  pfn_clEnqueueWriteBufferRect                 clEnqueueWriteBufferRect;
  pfn_clEnqueueCopyBufferRect                  clEnqueueCopyBufferRect;

  pfn_clCreateSubDevicesEXT                    clCreateSubDevicesEXT;
  pfn_clRetainDeviceEXT                        clRetainDeviceEXT;
  pfn_clReleaseDeviceEXT                       clReleaseDeviceEXT;

  pfn_clCreateEventFromGLsyncKHR               clCreateEventFromGLsyncKHR;

  pfn_clCreateSubDevices                       clCreateSubDevices;
  pfn_clRetainDevice                           clRetainDevice;
  pfn_clReleaseDevice                          clReleaseDevice;
  pfn_clCreateImage                            clCreateImage;
  pfn_clCreateProgramWithBuiltInKernels        clCreateProgramWithBuiltInKernels;
  pfn_clCompileProgram                         clCompileProgram;
  pfn_clLinkProgram                            clLinkProgram;
  pfn_clUnloadPlatformCompiler                 clUnloadPlatformCompiler;
  pfn_clGetKernelArgInfo                       clGetKernelArgInfo;
  pfn_clEnqueueFillBuffer                      clEnqueueFillBuffer;
  pfn_clEnqueueFillImage                       clEnqueueFillImage;
  pfn_clEnqueueMigrateMemObjects               clEnqueueMigrateMemObjects;
  pfn_clEnqueueMarkerWithWaitList              clEnqueueMarkerWithWaitList;
  pfn_clEnqueueBarrierWithWaitList             clEnqueueBarrierWithWaitList;
  pfn_clGetExtensionFunctionAddressForPlatform clGetExtensionFunctionAddressForPlatform;
  pfn_clCreateFromGLTexture                    clCreateFromGLTexture;

  pfn_clGetDeviceIDsFromD3D11KHR               clGetDeviceIDsFromD3D11KHR;
  pfn_clCreateFromD3D11BufferKHR               clCreateFromD3D11BufferKHR;
  pfn_clCreateFromD3D11Texture2DKHR            clCreateFromD3D11Texture2DKHR;
  pfn_clCreateFromD3D11Texture3DKHR            clCreateFromD3D11Texture3DKHR;
  pfn_clCreateFromDX9MediaSurfaceKHR           clCreateFromDX9MediaSurfaceKHR;
  pfn_clEnqueueAcquireD3D11ObjectsKHR          clEnqueueAcquireD3D11ObjectsKHR;
  pfn_clEnqueueReleaseD3D11ObjectsKHR          clEnqueueReleaseD3D11ObjectsKHR;

  pfn_clGetDeviceIDsFromDX9MediaAdapterKHR     clGetDeviceIDsFromDX9MediaAdapterKHR;
  pfn_clEnqueueAcquireDX9MediaSurfacesKHR      clEnqueueAcquireDX9MediaSurfacesKHR;
  pfn_clEnqueueReleaseDX9MediaSurfacesKHR      clEnqueueReleaseDX9MediaSurfacesKHR;
};

#ifdef __cplusplus
}
#endif

#endif // __SNUCL__ICD_H
