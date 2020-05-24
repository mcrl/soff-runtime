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

#include "CLDevice.h"
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <semaphore.h>
#include <CL/cl.h>
#include "CLCommand.h"
#include "CLDispatch.h"
#include "CLEvent.h"
#include "CLKernel.h"
#include "CLMem.h"
#include "CLObject.h"
#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLScheduler.h"
#include "Structs.h"
#include "Utils.h"

using namespace std;

#define READY_QUEUE_SIZE 4096

CLDevice::CLDevice(int node_id)
    : ready_queue_(READY_QUEUE_SIZE) {
  CLPlatform* platform = CLPlatform::GetPlatform();
  platform->AddDevice(this);
  scheduler_ = platform->AllocIdleScheduler();
  node_id_ = node_id;
  parent_ = NULL;

  sem_init(&sem_ready_queue_, 0, 0);
}

CLDevice::CLDevice(CLDevice* parent)
    : ready_queue_(READY_QUEUE_SIZE) {
  CLPlatform* platform = CLPlatform::GetPlatform();
  platform->AddDevice(this);
  scheduler_ = parent->scheduler_;
  node_id_ = parent->node_id_;
  parent_ = parent;

  sem_init(&sem_ready_queue_, 0, 0);

  /*
   * OpenCL 1.2 Specification rev 19
   * The device queries should return the same information for a root-level
   * device and any sub-devices created from this device except the following
   * queries:
   *     CL_DEVICE_GLOBAL_MEM_CACHE_SIZE
   *     CL_DEVICE_BUILT_IN_KERNELS
   *     CL_DEVICE_PARENT_DEVICE
   *     CL_DEVICE_PARTITION_TYPE
   *     CL_DEVICE_REFERENCE_COUNT
   *
   * This constructor initializes all properties except:
   *     global_mem_cache_size_
   *     built_in_kernels_
   *     partition_max_compute_units_
   *     partition_type_
   *     partition_type_len_
   */
  type_ = parent->type_;
  vendor_id_ = parent->vendor_id_;
  max_compute_units_ = parent->max_compute_units_;
  max_work_item_dimensions_ = parent->max_work_item_dimensions_;
  memcpy(max_work_item_sizes_, parent->max_work_item_sizes_,
         sizeof(size_t) * 3);
  max_work_group_size_ = parent->max_work_group_size_;

  preferred_vector_width_char_ = parent->preferred_vector_width_char_;
  preferred_vector_width_short_ = parent->preferred_vector_width_short_;
  preferred_vector_width_int_ = parent->preferred_vector_width_int_;
  preferred_vector_width_long_ = parent->preferred_vector_width_long_;
  preferred_vector_width_float_ = parent->preferred_vector_width_float_;
  preferred_vector_width_double_ = parent->preferred_vector_width_double_;
  preferred_vector_width_half_ = parent->preferred_vector_width_half_;
  native_vector_width_char_ = parent->native_vector_width_char_;
  native_vector_width_short_ = parent->native_vector_width_short_;
  native_vector_width_int_ = parent->native_vector_width_int_;
  native_vector_width_long_ = parent->native_vector_width_long_;
  native_vector_width_float_ = parent->native_vector_width_float_;
  native_vector_width_double_ = parent->native_vector_width_double_;
  native_vector_width_half_ = parent->native_vector_width_half_;
  max_clock_frequency_ = parent->max_clock_frequency_;
  address_bits_ = parent->address_bits_;

  max_mem_alloc_size_ = parent->max_mem_alloc_size_;

  image_support_ = parent->image_support_;
  max_read_image_args_ = parent->max_read_image_args_;
  max_write_image_args_ = parent->max_write_image_args_;
  image2d_max_width_ = parent->image2d_max_width_;
  image2d_max_height_ = parent->image2d_max_height_;
  image3d_max_width_ = parent->image3d_max_width_;
  image3d_max_height_ = parent->image3d_max_height_;
  image3d_max_depth_ = parent->image3d_max_depth_;
  image_max_buffer_size_ = parent->image_max_buffer_size_;
  image_max_array_size_ = parent->image_max_array_size_;
  max_samplers_ = parent->max_samplers_;

  max_parameter_size_ = parent->max_parameter_size_;
  mem_base_addr_align_ = parent->mem_base_addr_align_;
  min_data_type_align_size_ = parent->min_data_type_align_size_;

  single_fp_config_ = parent->single_fp_config_;
  double_fp_config_ = parent->double_fp_config_;

  global_mem_cache_type_ = parent->global_mem_cache_type_;
  global_mem_cacheline_size_ = parent->global_mem_cacheline_size_;
  global_mem_size_ = parent->global_mem_size_;

  max_constant_buffer_size_ = parent->max_constant_buffer_size_;
  max_constant_args_ = parent->max_constant_args_;

  local_mem_type_ = parent->local_mem_type_;
  local_mem_size_ = parent->local_mem_size_;
  error_correction_support_ = parent->error_correction_support_;

  host_unified_memory_ = parent->host_unified_memory_;

  profiling_timer_resolution_ = parent->profiling_timer_resolution_;

  endian_little_ = parent->endian_little_;
  available_ = parent->available_;

  compiler_available_ = parent->compiler_available_;
  linker_available_ = parent->linker_available_;

  execution_capabilities_ = parent->execution_capabilities_;

  queue_properties_ = parent->queue_properties_;

  name_ = parent->name_;
  vendor_ = parent->vendor_;
  driver_version_ = parent->driver_version_;
  profile_ = parent->profile_;
  device_version_ = parent->device_version_;
  opencl_c_version_ = parent->opencl_c_version_;
  device_extensions_ = parent->device_extensions_;

  printf_buffer_size_ = parent->printf_buffer_size_;

  preferred_interop_user_sync_ = parent->preferred_interop_user_sync_;

  partition_max_sub_devices_ = parent->partition_max_sub_devices_;
  memcpy(partition_properties_, parent->partition_properties_,
         sizeof(size_t) * 3);
  num_partition_properties_ = parent->num_partition_properties_;
  affinity_domain_ = parent->affinity_domain_;
}

void CLDevice::Cleanup() {
  CLPlatform* platform = CLPlatform::GetPlatform();
  platform->RemoveDevice(this);
}

CLDevice::~CLDevice() {
  sem_destroy(&sem_ready_queue_);
}

cl_int CLDevice::GetDeviceInfo(cl_device_info param_name,
                               size_t param_value_size, void* param_value,
                               size_t* param_value_size_ret) {
  switch (param_name) {
    GET_OBJECT_INFO(CL_DEVICE_TYPE, cl_device_type, type_);
    GET_OBJECT_INFO(CL_DEVICE_VENDOR_ID, cl_uint, vendor_id_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint, max_compute_units_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint,
                    max_work_item_dimensions_);
    GET_OBJECT_INFO_A(CL_DEVICE_MAX_WORK_ITEM_SIZES, size_t,
                      max_work_item_sizes_, max_work_item_dimensions_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t,
                    max_work_group_size_);

    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, cl_uint,
                    preferred_vector_width_char_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, cl_uint,
                    preferred_vector_width_short_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint,
                    preferred_vector_width_int_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint,
                    preferred_vector_width_long_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, cl_uint,
                    preferred_vector_width_float_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, cl_uint,
                    preferred_vector_width_double_);
    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint,
                    preferred_vector_width_half_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint,
                    native_vector_width_char_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint,
                    native_vector_width_short_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint,
                    native_vector_width_int_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint,
                    native_vector_width_long_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint,
                    native_vector_width_float_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint,
                    native_vector_width_double_);
    GET_OBJECT_INFO(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint,
                    native_vector_width_half_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint,
                    max_clock_frequency_);
    GET_OBJECT_INFO(CL_DEVICE_ADDRESS_BITS, cl_uint, address_bits_);

    GET_OBJECT_INFO(CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong,
                    max_mem_alloc_size_);

    GET_OBJECT_INFO(CL_DEVICE_IMAGE_SUPPORT, cl_bool, image_support_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_READ_IMAGE_ARGS, cl_uint,
                    max_read_image_args_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_WRITE_IMAGE_ARGS, cl_uint,
                    max_write_image_args_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE2D_MAX_WIDTH, size_t, image2d_max_width_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE2D_MAX_HEIGHT, size_t, image2d_max_height_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE3D_MAX_WIDTH, size_t, image3d_max_width_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_t, image3d_max_height_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE3D_MAX_DEPTH, size_t, image3d_max_depth_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, size_t,
                    image_max_buffer_size_);
    GET_OBJECT_INFO(CL_DEVICE_IMAGE_MAX_ARRAY_SIZE , size_t,
                    image_max_array_size_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_SAMPLERS, cl_uint, max_samplers_);

    GET_OBJECT_INFO(CL_DEVICE_MAX_PARAMETER_SIZE, size_t, max_parameter_size_);

    GET_OBJECT_INFO(CL_DEVICE_MEM_BASE_ADDR_ALIGN, cl_uint,
                    mem_base_addr_align_);
    GET_OBJECT_INFO(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, cl_uint,
                    min_data_type_align_size_);

    GET_OBJECT_INFO(CL_DEVICE_SINGLE_FP_CONFIG, cl_device_fp_config,
                    single_fp_config_);
    GET_OBJECT_INFO(CL_DEVICE_DOUBLE_FP_CONFIG, cl_device_fp_config,
                    double_fp_config_);

    GET_OBJECT_INFO(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, cl_device_mem_cache_type,
                    global_mem_cache_type_);
    GET_OBJECT_INFO(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, cl_uint,
                    global_mem_cacheline_size_);
    GET_OBJECT_INFO(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, cl_ulong,
                    global_mem_cache_size_);
    GET_OBJECT_INFO(CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong,
                    global_mem_size_);

    GET_OBJECT_INFO(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong,
                    max_constant_buffer_size_);
    GET_OBJECT_INFO(CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint, max_constant_args_);

    GET_OBJECT_INFO(CL_DEVICE_LOCAL_MEM_TYPE, cl_device_local_mem_type,
                    local_mem_type_);
    GET_OBJECT_INFO(CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong, local_mem_size_);
    GET_OBJECT_INFO(CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool,
                    error_correction_support_);

    GET_OBJECT_INFO(CL_DEVICE_HOST_UNIFIED_MEMORY, cl_bool,
                    host_unified_memory_);

    GET_OBJECT_INFO(CL_DEVICE_PROFILING_TIMER_RESOLUTION, size_t,
                    profiling_timer_resolution_);

    GET_OBJECT_INFO(CL_DEVICE_ENDIAN_LITTLE, cl_bool, endian_little_);
    GET_OBJECT_INFO(CL_DEVICE_AVAILABLE, cl_bool, available_);

    GET_OBJECT_INFO(CL_DEVICE_COMPILER_AVAILABLE, cl_bool,
                    compiler_available_);
    GET_OBJECT_INFO(CL_DEVICE_LINKER_AVAILABLE, cl_bool,
                    linker_available_);

    GET_OBJECT_INFO(CL_DEVICE_EXECUTION_CAPABILITIES,
                    cl_device_exec_capabilities, execution_capabilities_);

    GET_OBJECT_INFO(CL_DEVICE_QUEUE_PROPERTIES, cl_command_queue_properties,
                    queue_properties_);

    GET_OBJECT_INFO_S(CL_DEVICE_BUILT_IN_KERNELS, built_in_kernels_);

    GET_OBJECT_INFO_T(CL_DEVICE_PLATFORM, cl_platform_id,
                      CLPlatform::GetPlatform()->st_obj());

    GET_OBJECT_INFO_S(CL_DEVICE_NAME, name_);
    GET_OBJECT_INFO_S(CL_DEVICE_VENDOR, vendor_);
    GET_OBJECT_INFO_S(CL_DRIVER_VERSION, driver_version_);
    GET_OBJECT_INFO_S(CL_DEVICE_PROFILE, profile_);
    GET_OBJECT_INFO_S(CL_DEVICE_VERSION, device_version_);
    GET_OBJECT_INFO_S(CL_DEVICE_OPENCL_C_VERSION, opencl_c_version_);
    GET_OBJECT_INFO_S(CL_DEVICE_EXTENSIONS, device_extensions_);

    GET_OBJECT_INFO(CL_DEVICE_PRINTF_BUFFER_SIZE, size_t, printf_buffer_size_);

    GET_OBJECT_INFO(CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, cl_bool,
                    preferred_interop_user_sync_);

    GET_OBJECT_INFO_T(CL_DEVICE_PARENT_DEVICE, cl_device_id,
                      (parent_ == NULL ? NULL : parent_->st_obj()));
    GET_OBJECT_INFO(CL_DEVICE_PARTITION_MAX_SUB_DEVICES, cl_uint,
                    partition_max_sub_devices_);
    GET_OBJECT_INFO_A(CL_DEVICE_PARTITION_PROPERTIES,
                      cl_device_partition_property, partition_properties_,
                      num_partition_properties_);
    GET_OBJECT_INFO(CL_DEVICE_PARTITION_AFFINITY_DOMAIN,
                    cl_device_affinity_domain, affinity_domain_);
    GET_OBJECT_INFO_A(CL_DEVICE_PARTITION_TYPE, cl_device_partition_property,
                      partition_type_, partition_type_len_);

    GET_OBJECT_INFO_T(CL_DEVICE_REFERENCE_COUNT, cl_uint, ref_cnt());

    default: return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

cl_int CLDevice::CreateSubDevices(
    const cl_device_partition_property* properties, cl_uint num_devices,
    cl_device_id* out_devices, cl_uint* num_devices_ret) {
  if (properties == NULL) return CL_INVALID_VALUE;

  bool supported = false;
  for (size_t i = 0; i < num_partition_properties_; i++) {
    if (properties[0] == partition_properties_[i]) {
      supported = true;
      break;
    }
  }
  if (!supported) return CL_INVALID_VALUE;

  switch (properties[0]) {
    case CL_DEVICE_PARTITION_EQUALLY: {
      unsigned int n = (unsigned int)properties[1];
      if (n == 0) return CL_INVALID_VALUE;
      if (properties[2] != 0) return CL_INVALID_VALUE;

      return CreateSubDevicesEqually(n, num_devices, out_devices,
                                     num_devices_ret);
    }
    case CL_DEVICE_PARTITION_BY_COUNTS: {
      vector<unsigned int> sizes;
      unsigned int total_size = 0;
      size_t idx = 1;
      while (properties[idx] != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END) {
        unsigned int size = properties[idx];
        sizes.push_back(size);
        total_size += size;
      }

      if (sizes.empty()) return CL_INVALID_VALUE;
      if (properties[idx + 1] != 0) return CL_INVALID_VALUE;
      if (sizes.size() > partition_max_sub_devices_)
        return CL_INVALID_DEVICE_PARTITION_COUNT;
      if (total_size > partition_max_compute_units_)
        return CL_INVALID_DEVICE_PARTITION_COUNT;

      return CreateSubDevicesByCount(sizes, num_devices, out_devices,
                                     num_devices_ret);
    }
    case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN: {
      cl_device_affinity_domain domain =
          (cl_device_affinity_domain)properties[1];
      if (!(domain & (CL_DEVICE_AFFINITY_DOMAIN_NUMA |
                      CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE |
                      CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE |
                      CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE |
                      CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE |
                      CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE))) {
        return CL_INVALID_VALUE;
      }
      if (properties[2] != 0) return CL_INVALID_VALUE;

      return CreateSubDevicesByAffinity(domain, num_devices, out_devices,
                                        num_devices_ret);
    }
    default: return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

bool CLDevice::IsSupportedBuiltInKernels(const char* kernel_names) const {
  // Currently SnuCL does not support built-in kernels
  return false;
}

void CLDevice::SerializeDeviceInfo(void* buffer) {
  char* p = (char*)buffer;

#define SERIALIZE_INFO(type, value) \
  memcpy(p, &value, sizeof(type));  \
  p += sizeof(type);
#define SERIALIZE_INFO_A(type, value, length) \
  memcpy(p, value, sizeof(type) * length);    \
  p += sizeof(type) * length;
#define SERIALIZE_INFO_S(value, limit)  \
  {                                     \
    size_t length = strlen(value);      \
    if (length > limit)                 \
      length = limit;                   \
    memcpy(p, &length, sizeof(size_t)); \
    p += sizeof(size_t);                \
    memcpy(p, value, length);           \
    p += length;                        \
  }

  SERIALIZE_INFO(cl_uint, vendor_id_);
  SERIALIZE_INFO(cl_uint, max_compute_units_);
  SERIALIZE_INFO(cl_uint, max_work_item_dimensions_);
  SERIALIZE_INFO_A(size_t, max_work_item_sizes_, 3);
  SERIALIZE_INFO(size_t, max_work_group_size_);

  SERIALIZE_INFO(cl_uint, preferred_vector_width_char_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_short_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_int_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_long_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_float_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_double_);
  SERIALIZE_INFO(cl_uint, preferred_vector_width_half_);
  SERIALIZE_INFO(cl_uint, native_vector_width_char_);
  SERIALIZE_INFO(cl_uint, native_vector_width_short_);
  SERIALIZE_INFO(cl_uint, native_vector_width_int_);
  SERIALIZE_INFO(cl_uint, native_vector_width_long_);
  SERIALIZE_INFO(cl_uint, native_vector_width_float_);
  SERIALIZE_INFO(cl_uint, native_vector_width_double_);
  SERIALIZE_INFO(cl_uint, native_vector_width_half_);
  SERIALIZE_INFO(cl_uint, max_clock_frequency_);
  SERIALIZE_INFO(cl_uint, address_bits_);
  
  SERIALIZE_INFO(cl_ulong, max_mem_alloc_size_);
  
  SERIALIZE_INFO(cl_bool, image_support_);
  SERIALIZE_INFO(cl_uint, max_read_image_args_);
  SERIALIZE_INFO(cl_uint, max_write_image_args_);
  SERIALIZE_INFO(size_t, image2d_max_width_);
  SERIALIZE_INFO(size_t, image2d_max_height_);
  SERIALIZE_INFO(size_t, image3d_max_width_);
  SERIALIZE_INFO(size_t, image3d_max_height_);
  SERIALIZE_INFO(size_t, image3d_max_depth_);
  SERIALIZE_INFO(size_t, image_max_buffer_size_);
  SERIALIZE_INFO(size_t, image_max_array_size_);
  SERIALIZE_INFO(cl_uint, max_samplers_);
  
  SERIALIZE_INFO(size_t, max_parameter_size_);
  SERIALIZE_INFO(cl_uint, mem_base_addr_align_);
  SERIALIZE_INFO(cl_uint, min_data_type_align_size_);
  
  SERIALIZE_INFO(cl_device_fp_config, single_fp_config_);
  SERIALIZE_INFO(cl_device_fp_config, double_fp_config_);
  
  SERIALIZE_INFO(cl_device_mem_cache_type, global_mem_cache_type_);
  SERIALIZE_INFO(cl_uint, global_mem_cacheline_size_);
  SERIALIZE_INFO(cl_ulong, global_mem_cache_size_);
  SERIALIZE_INFO(cl_ulong, global_mem_size_);
  
  SERIALIZE_INFO(cl_ulong, max_constant_buffer_size_);
  SERIALIZE_INFO(cl_uint, max_constant_args_);
  
  SERIALIZE_INFO(cl_device_local_mem_type, local_mem_type_);
  SERIALIZE_INFO(cl_ulong, local_mem_size_);
  SERIALIZE_INFO(cl_bool, error_correction_support_);
  
  SERIALIZE_INFO(cl_bool, host_unified_memory_);
  
  SERIALIZE_INFO(size_t, profiling_timer_resolution_);
  
  SERIALIZE_INFO(cl_bool, endian_little_);
  SERIALIZE_INFO(cl_bool, available_);
  
  SERIALIZE_INFO(cl_bool, compiler_available_);
  SERIALIZE_INFO(cl_bool, linker_available_);
  
  SERIALIZE_INFO(cl_device_exec_capabilities, execution_capabilities_);
  
  SERIALIZE_INFO(cl_command_queue_properties, queue_properties_);

  SERIALIZE_INFO_S(built_in_kernels_, 0);
  
  SERIALIZE_INFO_S(name_, 64);
  SERIALIZE_INFO_S(vendor_, 64);
  SERIALIZE_INFO_S(driver_version_, 64);
  SERIALIZE_INFO_S(profile_, 32);
  SERIALIZE_INFO_S(device_version_, 32);
  SERIALIZE_INFO_S(opencl_c_version_, 32);
  SERIALIZE_INFO_S(device_extensions_, 1024);
  
  SERIALIZE_INFO(size_t, printf_buffer_size_);
  
  SERIALIZE_INFO(cl_bool, preferred_interop_user_sync_);

  SERIALIZE_INFO(cl_uint, partition_max_sub_devices_);
  SERIALIZE_INFO(cl_uint, partition_max_compute_units_);
  SERIALIZE_INFO_A(cl_device_partition_property, partition_properties_, 3);
  SERIALIZE_INFO(size_t, num_partition_properties_);
  SERIALIZE_INFO(cl_device_affinity_domain, affinity_domain_);

#undef SERIALIZE_INFO
#undef SERIALIZE_INFO_A
#undef SERIALIZE_INFO_S
}

void CLDevice::DeserializeDeviceInfo(void* buffer) {
  char* p = (char*)buffer;

#define DESERIALIZE_INFO(type, value) \
  memcpy(&value, p, sizeof(type));  \
  p += sizeof(type);
#define DESERIALIZE_INFO_A(type, value, length) \
  memcpy(value, p, sizeof(type) * length);    \
  p += sizeof(type) * length;
#define DESERIALIZE_INFO_S(value)        \
  {                                      \
    size_t length;                       \
    memcpy(&length, p, sizeof(size_t));  \
    p += sizeof(size_t);                 \
    char* s = (char*)malloc(length + 1); \
    memcpy(s, p, length);                \
    p += length;                         \
    s[length] = '\0';                    \
    value = s;                           \
  }

  DESERIALIZE_INFO(cl_uint, vendor_id_);
  DESERIALIZE_INFO(cl_uint, max_compute_units_);
  DESERIALIZE_INFO(cl_uint, max_work_item_dimensions_);
  DESERIALIZE_INFO_A(size_t, max_work_item_sizes_, 3);
  DESERIALIZE_INFO(size_t, max_work_group_size_);

  DESERIALIZE_INFO(cl_uint, preferred_vector_width_char_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_short_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_int_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_long_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_float_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_double_);
  DESERIALIZE_INFO(cl_uint, preferred_vector_width_half_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_char_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_short_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_int_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_long_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_float_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_double_);
  DESERIALIZE_INFO(cl_uint, native_vector_width_half_);
  DESERIALIZE_INFO(cl_uint, max_clock_frequency_);
  DESERIALIZE_INFO(cl_uint, address_bits_);
  
  DESERIALIZE_INFO(cl_ulong, max_mem_alloc_size_);
  
  DESERIALIZE_INFO(cl_bool, image_support_);
  DESERIALIZE_INFO(cl_uint, max_read_image_args_);
  DESERIALIZE_INFO(cl_uint, max_write_image_args_);
  DESERIALIZE_INFO(size_t, image2d_max_width_);
  DESERIALIZE_INFO(size_t, image2d_max_height_);
  DESERIALIZE_INFO(size_t, image3d_max_width_);
  DESERIALIZE_INFO(size_t, image3d_max_height_);
  DESERIALIZE_INFO(size_t, image3d_max_depth_);
  DESERIALIZE_INFO(size_t, image_max_buffer_size_);
  DESERIALIZE_INFO(size_t, image_max_array_size_);
  DESERIALIZE_INFO(cl_uint, max_samplers_);
  
  DESERIALIZE_INFO(size_t, max_parameter_size_);
  DESERIALIZE_INFO(cl_uint, mem_base_addr_align_);
  DESERIALIZE_INFO(cl_uint, min_data_type_align_size_);
  
  DESERIALIZE_INFO(cl_device_fp_config, single_fp_config_);
  DESERIALIZE_INFO(cl_device_fp_config, double_fp_config_);
  
  DESERIALIZE_INFO(cl_device_mem_cache_type, global_mem_cache_type_);
  DESERIALIZE_INFO(cl_uint, global_mem_cacheline_size_);
  DESERIALIZE_INFO(cl_ulong, global_mem_cache_size_);
  DESERIALIZE_INFO(cl_ulong, global_mem_size_);
  
  DESERIALIZE_INFO(cl_ulong, max_constant_buffer_size_);
  DESERIALIZE_INFO(cl_uint, max_constant_args_);
  
  DESERIALIZE_INFO(cl_device_local_mem_type, local_mem_type_);
  DESERIALIZE_INFO(cl_ulong, local_mem_size_);
  DESERIALIZE_INFO(cl_bool, error_correction_support_);
  
  DESERIALIZE_INFO(cl_bool, host_unified_memory_);
  
  DESERIALIZE_INFO(size_t, profiling_timer_resolution_);
  
  DESERIALIZE_INFO(cl_bool, endian_little_);
  DESERIALIZE_INFO(cl_bool, available_);
  
  DESERIALIZE_INFO(cl_bool, compiler_available_);
  DESERIALIZE_INFO(cl_bool, linker_available_);
  
  DESERIALIZE_INFO(cl_device_exec_capabilities, execution_capabilities_);
  
  DESERIALIZE_INFO(cl_command_queue_properties, queue_properties_);

  DESERIALIZE_INFO_S(built_in_kernels_);
  
  DESERIALIZE_INFO_S(name_);
  DESERIALIZE_INFO_S(vendor_);
  DESERIALIZE_INFO_S(driver_version_);
  DESERIALIZE_INFO_S(profile_);
  DESERIALIZE_INFO_S(device_version_);
  DESERIALIZE_INFO_S(opencl_c_version_);
  DESERIALIZE_INFO_S(device_extensions_);
  
  DESERIALIZE_INFO(size_t, printf_buffer_size_);
  
  DESERIALIZE_INFO(cl_bool, preferred_interop_user_sync_);

  DESERIALIZE_INFO(cl_uint, partition_max_sub_devices_);
  DESERIALIZE_INFO(cl_uint, partition_max_compute_units_);
  DESERIALIZE_INFO_A(cl_device_partition_property, partition_properties_, 3);
  DESERIALIZE_INFO(size_t, num_partition_properties_);
  DESERIALIZE_INFO(cl_device_affinity_domain, affinity_domain_);

#undef DESERIALIZE_INFO
#undef DESERIALIZE_INFO_A
#undef DESERIALIZE_INFO_S
}

int CLDevice::GetDistance(CLDevice* other) const {
  if (this == other) {
    return 0;
  } else if (other == LATEST_HOST) {
    if (node_id_ == 0) {
      if (type_ == CL_DEVICE_TYPE_CPU)
        return 1;
      else
        return 2;
    } else if (type_ == CL_DEVICE_TYPE_CPU) {
      return 4;
    } else {
      return 5;
    }
  } else if (node_id_ == other->node_id_) {
    if (type_ == CL_DEVICE_TYPE_CPU && other->type_ == CL_DEVICE_TYPE_CPU)
      return 1;
    else if (type_ == CL_DEVICE_TYPE_CPU || other->type_ == CL_DEVICE_TYPE_CPU)
      return 2;
    else
      return 3;
  } else if (type_ == CL_DEVICE_TYPE_CPU &&
             other->type_ == CL_DEVICE_TYPE_CPU) {
    return 4;
  } else if (type_ == CL_DEVICE_TYPE_CPU ||
             other->type_ == CL_DEVICE_TYPE_CPU) {
    return 5;
  } else {
    return 6;
  }
}

void CLDevice::JoinSupportedImageSize(size_t& image2d_max_width,
                                      size_t& image2d_max_height,
                                      size_t& image3d_max_width,
                                      size_t& image3d_max_height,
                                      size_t& image3d_max_depth,
                                      size_t& image_max_buffer_size,
                                      size_t& image_max_array_size) const {
  if (image2d_max_width > image2d_max_width_)
    image2d_max_width = image2d_max_width_;
  if (image2d_max_height > image2d_max_height_)
    image2d_max_height = image2d_max_height_;
  if (image3d_max_width > image3d_max_width_)
    image3d_max_width = image3d_max_width_;
  if (image3d_max_height > image3d_max_height_)
    image3d_max_height = image3d_max_height_;
  if (image3d_max_depth > image3d_max_depth_)
    image3d_max_depth = image3d_max_depth_;
  if (image_max_buffer_size > image_max_buffer_size_)
    image_max_buffer_size = image_max_buffer_size_;
  if (image_max_array_size > image_max_array_size_)
    image_max_array_size = image_max_array_size_;
}

void CLDevice::AddCommandQueue(CLCommandQueue* queue) {
  scheduler_->AddCommandQueue(queue);
}

void CLDevice::RemoveCommandQueue(CLCommandQueue* queue) {
  scheduler_->RemoveCommandQueue(queue);
}

void CLDevice::InvokeScheduler() {
  scheduler_->Invoke();
}

void CLDevice::EnqueueReadyQueue(CLCommand* command) {
  while (!ready_queue_.Enqueue(command)) {}
  sem_post(&sem_ready_queue_);
}

CLCommand* CLDevice::DequeueReadyQueue() {
  CLCommand* command;
  if (ready_queue_.Dequeue(&command))
    return command;
  else
    return NULL;
}

void CLDevice::InvokeReadyQueue() {
  sem_post(&sem_ready_queue_);
}

void CLDevice::WaitReadyQueue() {
  sem_wait(&sem_ready_queue_);
}

CLEvent* CLDevice::EnqueueBuildProgram(CLProgram* program,
                                       CLProgramSource* source,
                                       CLProgramBinary* binary,
                                       const char* options) {
  CLCommand* command = CLCommand::CreateBuildProgram(this, program, source,
                                                     binary, options);
  CLEvent* event = command->ExportEvent();
  EnqueueReadyQueue(command);
  return event;
}

CLEvent* CLDevice::EnqueueCompileProgram(CLProgram* program,
                                         CLProgramSource* source,
                                         const char* options,
                                         vector<CLProgramSource*>& headers) {
  CLCommand* command = CLCommand::CreateCompileProgram(this, program, source,
                                                       options, headers);
  CLEvent* event = command->ExportEvent();
  EnqueueReadyQueue(command);
  return event;
}

CLEvent* CLDevice::EnqueueLinkProgram(CLProgram* program,
                                      vector<CLProgramBinary*>& binaries,
                                      const char* options) {
  CLCommand* command = CLCommand::CreateLinkProgram(this, program, binaries,
                                                    options);
  CLEvent* event = command->ExportEvent();
  EnqueueReadyQueue(command);
  return event;
}

void CLDevice::MapBuffer(CLCommand* command, CLMem* mem_src,
                         cl_map_flags map_flags, size_t off_src, size_t size,
                         void* mapped_ptr) {
  if ((map_flags & (CL_MAP_READ | CL_MAP_WRITE)) &&
      mem_src->IsMapInitRequired(mapped_ptr)) {
    ReadBuffer(NULL, mem_src, off_src, size, mapped_ptr);
  }
}

void CLDevice::MapImage(CLCommand* command, CLMem* mem_src,
                        cl_map_flags map_flags, size_t src_origin[3],
                        size_t region[3], void* mapped_ptr) {
  if ((map_flags & (CL_MAP_READ | CL_MAP_WRITE)) &&
      mem_src->IsMapInitRequired(mapped_ptr)) {
    ReadImage(NULL, mem_src, src_origin, region, 0, 0, mapped_ptr);
  }
}

void CLDevice::UnmapMemObject(CLCommand* command, CLMem* mem_src,
                              void* mapped_ptr) {
  if (mem_src->IsImage()) {
    size_t origin[3], region[3];
    if (mem_src->GetMapWritebackLayoutForImage(mapped_ptr, origin, region)) {
      WriteImage(NULL, mem_src, origin, region, 0, 0, mapped_ptr);
    }
  } else {
    size_t offset, size;
    if (mem_src->GetMapWritebackLayoutForBuffer(mapped_ptr, &offset, &size)) {
      WriteBuffer(NULL, mem_src, offset, size, mapped_ptr);
    }
  }
  mem_src->Unmap(mapped_ptr);
}

void CLDevice::MigrateMemObjects(CLCommand* command, cl_uint num_mem_objects,
                                 CLMem** mem_list,
                                 cl_mem_migration_flags flags) {
  for (cl_uint i = 0; i < num_mem_objects; i++) {
    mem_list[i]->GetDevSpecific(this);
  }
}

bool CLDevice::IsComplete(CLCommand* command) {
  return true;
}

void* CLDevice::AllocSampler(CLSampler* sampler) {
  return NULL;
}

void CLDevice::FreeSampler(CLSampler* sampler, void* dev_specific) {
  // Do nothing
}

void* CLDevice::AllocKernel(CLKernel* kernel) {
  return NULL;
}

void CLDevice::FreeKernel(CLKernel* kernel, void* dev_specific) {
  // Do nothing
}

void CLDevice::FreeFile(CLFile* file) {
  // Do nothing
}

cl_int CLDevice::CreateSubDevicesEqually(unsigned int n, cl_uint num_devices,
                                         cl_device_id* out_devices,
                                         cl_uint* num_devices_ret) {
  return CL_DEVICE_PARTITION_FAILED;
}

cl_int CLDevice::CreateSubDevicesByCount(const vector<unsigned int>& sizes,
                                         cl_uint num_devices,
                                         cl_device_id* out_devices,
                                         cl_uint* num_devices_ret) {
  return CL_DEVICE_PARTITION_FAILED;
}

cl_int CLDevice::CreateSubDevicesByAffinity(cl_device_affinity_domain domain,
                                            cl_uint num_devices,
                                            cl_device_id* out_devices,
                                            cl_uint* num_devices_ret) {
  return CL_DEVICE_PARTITION_FAILED;
}
