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

#ifndef __SNUCL__CL_DEVICE_H
#define __SNUCL__CL_DEVICE_H

#include <map>
#include <vector>
#include <semaphore.h>
#include <CL/cl.h>
#include <CL/cl_ext_snucl.h>
#include "CLKernel.h"
#include "CLObject.h"
#include "Structs.h"
#include "Utils.h"

class CLCommand;
class CLCommandQueue;
class CLEvent;
class CLFile;
class CLKernel;
class CLKernelArgs;
class CLPlatform;
class CLProgram;
class CLProgramBinary;
class CLProgramSource;
class CLScheduler;

class CLDevice: public CLObject<struct _cl_device_id, CLDevice> {
 public:
  CLDevice(int node_id);
  CLDevice(CLDevice* parent);
  virtual void Cleanup();
  virtual ~CLDevice();

  cl_device_type type() const { return type_; }
  int node_id() const { return node_id_; }

  cl_int GetDeviceInfo(cl_device_info param_name, size_t param_value_size,
                       void* param_value, size_t* param_value_size_ret);
  cl_int CreateSubDevices(const cl_device_partition_property* properties,
                          cl_uint num_devices, cl_device_id* out_devices,
                          cl_uint* num_devices_ret);

  bool IsImageSupported() const { return image_support_; }
  bool IsAvailable() const { return available_; }
  bool IsCompilerAvailable() const { return compiler_available_; }
  bool IsLinkerAvailable() const { return linker_available_; }
  bool IsSubDevice() const { return (parent_ != NULL); }
  bool IsSupportedBuiltInKernels(const char* kernel_names) const;

  void SerializeDeviceInfo(void* buffer);
  void DeserializeDeviceInfo(void* buffer);

  int GetDistance(CLDevice* other) const;

  void JoinSupportedImageSize(size_t& image2d_max_width,
                              size_t& image2d_max_height,
                              size_t& image3d_max_width,
                              size_t& image3d_max_height,
                              size_t& image3d_max_depth,
                              size_t& image_max_buffer_size,
                              size_t& image_max_array_size) const;

  void AddCommandQueue(CLCommandQueue* queue);
  void RemoveCommandQueue(CLCommandQueue* queue);
  void InvokeScheduler();

  void EnqueueReadyQueue(CLCommand* command);
  CLCommand* DequeueReadyQueue();
  void InvokeReadyQueue();
  void WaitReadyQueue();

  CLEvent* EnqueueBuildProgram(CLProgram* program, CLProgramSource* source,
                               CLProgramBinary* binary, const char* options);
  CLEvent* EnqueueCompileProgram(CLProgram* program, CLProgramSource* source,
                                 const char* options,
                                 std::vector<CLProgramSource*>& headers);
  CLEvent* EnqueueLinkProgram(CLProgram* program,
                              std::vector<CLProgramBinary*>& binaries,
                              const char* options);

  virtual void LaunchKernel(CLCommand* command, CLKernel* kernel,
                            cl_uint work_dim, size_t gwo[3], size_t gws[3],
                            size_t lws[3], size_t nwg[3],
                            std::map<cl_uint, CLKernelArg*>* kernel_args) = 0;
  virtual void LaunchNativeKernel(CLCommand* command, void (*user_func)(void*),
                                  void* native_args, size_t size,
                                  cl_uint num_mem_objects, CLMem** mem_list,
                                  ptrdiff_t* mem_offsets) = 0;
  virtual void ReadBuffer(CLCommand* command, CLMem* mem_src, size_t off_src,
                          size_t size, void* ptr) = 0;
  virtual void WriteBuffer(CLCommand* command, CLMem* mem_dst, size_t off_dst,
                           size_t size, void* ptr) = 0;
  virtual void CopyBuffer(CLCommand* command, CLMem* mem_src, CLMem* mem_dst,
                          size_t off_src, size_t off_dst, size_t size) = 0;
  virtual void ReadImage(CLCommand* command, CLMem* mem_src,
                         size_t src_origin[3], size_t region[3],
                         size_t dst_row_pitch, size_t dst_slice_pitch,
                         void* ptr) = 0;
  virtual void WriteImage(CLCommand* command, CLMem* mem_dst,
                          size_t dst_origin[3], size_t region[3],
                          size_t src_row_pitch, size_t src_slice_pitch,
                          void* ptr) = 0;
  virtual void CopyImage(CLCommand* command, CLMem* mem_src, CLMem* mem_dst,
                         size_t src_origin[3], size_t dst_origin[3],
                         size_t region[3]) = 0;
  virtual void CopyImageToBuffer(CLCommand* command, CLMem* mem_src,
                                 CLMem* mem_dst, size_t src_origin[3],
                                 size_t region[3], size_t off_dst) = 0;
  virtual void CopyBufferToImage(CLCommand* command, CLMem* mem_src,
                                 CLMem* mem_dst, size_t off_src,
                                 size_t dst_origin[3], size_t region[3]) = 0;
  virtual void MapBuffer(CLCommand* command, CLMem* mem_src,
                         cl_map_flags map_flags, size_t off_src, size_t size,
                         void* mapped_ptr);
  virtual void MapImage(CLCommand* command, CLMem* mem_src,
                        cl_map_flags map_flags, size_t src_origin[3],
                        size_t region[3], void* mapped_ptr);
  virtual void UnmapMemObject(CLCommand* command, CLMem* mem_src,
                              void* mapped_ptr);
  virtual void MigrateMemObjects(CLCommand* command, cl_uint num_mem_objects,
                                 CLMem** mem_list,
                                 cl_mem_migration_flags flags);
  virtual void ReadBufferRect(CLCommand* command, CLMem* mem_src,
                              size_t src_origin[3], size_t dst_origin[3],
                              size_t region[3], size_t src_row_pitch,
                              size_t src_slice_pitch, size_t dst_row_pitch,
                              size_t dst_slice_pitch, void* ptr) = 0;
  virtual void WriteBufferRect(CLCommand* command, CLMem* mem_dst,
                               size_t src_origin[3], size_t dst_origin[3],
                               size_t region[3], size_t src_row_pitch,
                               size_t src_slice_pitch, size_t dst_row_pitch,
                               size_t dst_slice_pitch, void* ptr) = 0;
  virtual void CopyBufferRect(CLCommand* command, CLMem* mem_src,
                              CLMem* mem_dst, size_t src_origin[3],
                              size_t dst_origin[3], size_t region[3],
                              size_t src_row_pitch, size_t src_slice_pitch,
                              size_t dst_row_pitch,
                              size_t dst_slice_pitch) = 0;
  virtual void FillBuffer(CLCommand* command, CLMem* mem_dst, void* pattern,
                          size_t pattern_size, size_t off_dst,
                          size_t size) = 0;
  virtual void FillImage(CLCommand* command, CLMem* mem_dst, void* fill_color,
                         size_t dst_origin[3], size_t region[3]) = 0;

  virtual void BuildProgram(CLCommand* command, CLProgram* program,
                            CLProgramSource* source, CLProgramBinary* binary,
                            const char* options) = 0;
  virtual void CompileProgram(CLCommand* command, CLProgram* program,
                              CLProgramSource* source, const char* options,
                              size_t num_headers,
                              CLProgramSource** headers) = 0;
  virtual void LinkProgram(CLCommand* command, CLProgram* program,
                           size_t num_binaries, CLProgramBinary** binaries,
                           const char* options) = 0;

  virtual void AlltoAllBuffer(CLCommand* command, CLMem* mem_src,
                              CLMem* mem_dst, size_t off_src, size_t off_dst,
                              size_t size) {}
  virtual void BroadcastBuffer(CLCommand* command, CLMem* mem_src,
                               CLMem* mem_dst, size_t off_src, size_t off_dst,
                               size_t size) {}

  virtual void LocalFileOpen(CLCommand* comamnd, CLFile* file,
                             const char* filename,
                             cl_file_open_flags open_flags) {}
  virtual void CopyBufferToFile(CLCommand* command, CLMem* mem_src,
                                CLFile* file_dst, size_t off_src,
                                size_t off_dst, size_t size) {}
  virtual void CopyFileToBuffer(CLCommand* command, CLFile* file_src,
                                CLMem* mem_dst, size_t off_src,
                                size_t off_dst, size_t size) {}

  virtual bool IsComplete(CLCommand* command);

  virtual void* AllocMem(CLMem* mem) = 0;
  virtual void FreeMem(CLMem* mem, void* dev_specific) = 0;
  virtual void* AllocSampler(CLSampler* sampler);
  virtual void FreeSampler(CLSampler* sampler, void* dev_specific);

  virtual void FreeExecutable(CLProgram* program, void* executable) = 0;
  virtual void* AllocKernel(CLKernel* kernel);
  virtual void FreeKernel(CLKernel* kernel, void* dev_specific);

  virtual void FreeFile(CLFile* file);

  virtual cl_int CreateSubDevicesEqually(unsigned int n, cl_uint num_devices,
                                         cl_device_id* out_devices,
                                         cl_uint* num_devices_ret);
  virtual cl_int CreateSubDevicesByCount(
      const std::vector<unsigned int>& sizes, cl_uint num_devices,
      cl_device_id* out_devices, cl_uint* num_devices_ret);
  virtual cl_int CreateSubDevicesByAffinity(cl_device_affinity_domain domain,
                                            cl_uint num_devices,
                                            cl_device_id* out_devices,
                                            cl_uint* num_devices_ret);

 protected:
  CLScheduler* scheduler_;
  LockFreeQueueMS ready_queue_;
  int node_id_;
  CLDevice* parent_;

  sem_t sem_ready_queue_;

  cl_device_type type_;
  cl_uint vendor_id_;
  cl_uint max_compute_units_;
  cl_uint max_work_item_dimensions_;
  size_t max_work_item_sizes_[3];
  size_t max_work_group_size_;

  cl_uint preferred_vector_width_char_;
  cl_uint preferred_vector_width_short_;
  cl_uint preferred_vector_width_int_;
  cl_uint preferred_vector_width_long_;
  cl_uint preferred_vector_width_float_;
  cl_uint preferred_vector_width_double_;
  cl_uint preferred_vector_width_half_;
  cl_uint native_vector_width_char_;
  cl_uint native_vector_width_short_;
  cl_uint native_vector_width_int_;
  cl_uint native_vector_width_long_;
  cl_uint native_vector_width_float_;
  cl_uint native_vector_width_double_;
  cl_uint native_vector_width_half_;
  cl_uint max_clock_frequency_;
  cl_uint address_bits_;

  cl_ulong max_mem_alloc_size_;

  cl_bool image_support_;
  cl_uint max_read_image_args_;
  cl_uint max_write_image_args_;
  size_t image2d_max_width_;
  size_t image2d_max_height_;
  size_t image3d_max_width_;
  size_t image3d_max_height_;
  size_t image3d_max_depth_;
  size_t image_max_buffer_size_;
  size_t image_max_array_size_;
  cl_uint max_samplers_;

  size_t max_parameter_size_;
  cl_uint mem_base_addr_align_;
  cl_uint min_data_type_align_size_;

  cl_device_fp_config single_fp_config_;
  cl_device_fp_config double_fp_config_;

  cl_device_mem_cache_type global_mem_cache_type_;
  cl_uint global_mem_cacheline_size_;
  cl_ulong global_mem_cache_size_;
  cl_ulong global_mem_size_;

  cl_ulong max_constant_buffer_size_;
  cl_uint max_constant_args_;

  cl_device_local_mem_type local_mem_type_;
  cl_ulong local_mem_size_;
  cl_bool error_correction_support_;

  cl_bool host_unified_memory_;

  size_t profiling_timer_resolution_;

  cl_bool endian_little_;
  cl_bool available_;

  cl_bool compiler_available_;
  cl_bool linker_available_;

  cl_device_exec_capabilities execution_capabilities_;

  cl_command_queue_properties queue_properties_;

  const char* built_in_kernels_;

  const char* name_;
  const char* vendor_;
  const char* driver_version_;
  const char* profile_;
  const char* device_version_;
  const char* opencl_c_version_;
  const char* device_extensions_;

  size_t printf_buffer_size_;

  cl_bool preferred_interop_user_sync_;

  cl_uint partition_max_sub_devices_;
  cl_uint partition_max_compute_units_;
  cl_device_partition_property partition_properties_[3];
  size_t num_partition_properties_;
  cl_device_affinity_domain affinity_domain_;
  cl_device_partition_property* partition_type_;
  size_t partition_type_len_;
};

#endif // __SNUCL__CL_DEVICE_H
