/*****************************************************************************/
/*                                                                           */
/* Copyright (c) 2020 Seoul National University.                             */
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
/* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                               */
/*                                                                           */
/* Contact information:                                                      */
/*   Center for Manycore Programming                                         */
/*   Department of Computer Science and Engineering                          */
/*   Seoul National University, Seoul 08826, Korea                           */
/*   http://aces.snu.ac.kr                                                   */
/*                                                                           */
/* Contributors:                                                             */
/*   Gangwon Jo, Heehoon Kim, Jeesoo Lee, and Jaejin Lee                     */
/*                                                                           */
/*****************************************************************************/

#include "opae/OPAEDevice.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CL/cl.h>
#include "CLCommand.h"
#include "CLDevice.h"
#include "CLKernel.h"
#include "CLMem.h"
#include "CLPlatform.h"
#include "CLProgram.h"
#include "CLSampler.h"
#include "Utils.h"
#include <opae/fpga.h>

using namespace std;

#define CHECK_ERROR(err) \
  do { \
    if (err != FPGA_OK) { \
      SNUCL_ERROR("OPAE error: %s (code = %d)\n", fpgaErrStr(err), err); \
      exit(1); \
    } \
  } while (false)

#define SNUCL_ERROR_EXIT(fmt, ...) \
  do { \
    SNUCL_ERROR(fmt, ##__VA_ARGS__); \
    exit(1); \
  } while (false)

struct OPAEBitstream {
  const unsigned char *data;
  size_t size;
  unsigned int frequency; // in KHz
};

OPAEDevice::OPAEDevice(fpga_token dev_token, fpga_token acc_token, OPAE_DEVICE_TYPE opae_device_type)
    : CLDevice(0), opae_device_token_(dev_token), opae_accelerator_token_(acc_token) {
  fpga_result err = FPGA_OK;

  type_ = CL_DEVICE_TYPE_ACCELERATOR;
  vendor_id_ = 201110;
  max_compute_units_ = 1024;
  max_work_item_dimensions_ = 3;
  max_work_item_sizes_[0] = max_work_item_sizes_[1] =
      max_work_item_sizes_[2] = 1024;
  max_work_group_size_ = 1024;

  preferred_vector_width_char_ = 1;
  preferred_vector_width_short_ = 1;
  preferred_vector_width_int_ = 1;
  preferred_vector_width_long_ = 1;
  preferred_vector_width_float_ = 1;
  preferred_vector_width_double_ = 1;
  preferred_vector_width_half_ = 1;
  native_vector_width_char_ = 1;
  native_vector_width_short_ = 1;
  native_vector_width_int_ = 1;
  native_vector_width_long_ = 1;
  native_vector_width_float_ = 1;
  native_vector_width_double_ = 1;
  native_vector_width_half_ = 1;
  max_clock_frequency_ = 320;
  address_bits_ = 64;

  image_support_ = CL_FALSE;
  max_read_image_args_ = 0;
  max_write_image_args_ = 0;
  image2d_max_width_ = 8192;
  image2d_max_height_ = 8192;
  image3d_max_width_ = 2048;
  image3d_max_height_ = 2048;
  image3d_max_depth_ = 2048;
  image_max_buffer_size_ = 65536;
  image_max_array_size_ = 65536;
  max_samplers_ = 0;

  max_parameter_size_ = 512;
  mem_base_addr_align_ = 1024;
  min_data_type_align_size_ = 1;

  single_fp_config_ = CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST |
                      CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_FMA;
                 // | CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT | CL_FP_SOFT_FLOAT;
  double_fp_config_ = CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST |
                      CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_FMA;

  global_mem_cache_type_ = CL_READ_WRITE_CACHE;
  global_mem_cacheline_size_ = 64;
  global_mem_cache_size_ = 64 * 1024;

  max_constant_buffer_size_ = 64 * 1024;
  max_constant_args_ = 8;

  local_mem_type_ = CL_LOCAL;
  local_mem_size_ = 16 * 1024;
  error_correction_support_ = CL_FALSE;

  host_unified_memory_ = CL_FALSE;

  profiling_timer_resolution_ = 1;

  endian_little_ = CL_TRUE;
  available_ = CL_TRUE;

  compiler_available_ = CL_TRUE;
  linker_available_ = CL_TRUE;

  execution_capabilities_ = CL_EXEC_KERNEL;
  queue_properties_ = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                      CL_QUEUE_PROFILING_ENABLE;

  built_in_kernels_ = "";

  switch (opae_device_type) {
    case OPAE_DEVICE_A10:
      name_ = "SOFF Device: Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA";
      break;
    case OPAE_DEVICE_D5005:
      name_ = "SOFF Device: Intel FPGA Programmable Acceleration Card D5005";
      break;
    default:
      name_ = "";
      break;
  }
  vendor_ = "Seoul National University";
  driver_version_ = "SOFF";
  profile_ = "FULL_PROFILE";
  device_version_ = "OpenCL 1.2 rev01";
  opencl_c_version_ = "OpenCL 1.2 rev01";
  device_extensions_ = "cl_khr_global_int32_base_atomics "
                       "cl_khr_global_int32_extended_atomics "
                       "cl_khr_local_int32_base_atomics "
                       "cl_khr_local_int32_extended_atomics "
                       "cl_khr_byte_addressable_store "
                       "cl_khr_fp64";

  printf_buffer_size_ = 1024 * 1024;

  preferred_interop_user_sync_ = CL_FALSE;

  partition_max_sub_devices_ = 1;
  partition_max_compute_units_ = max_compute_units_;
  num_partition_properties_ = 0;
  affinity_domain_ = 0; 
  partition_type_ = NULL;
  partition_type_len_ = 0;

  switch (opae_device_type) {
    case OPAE_DEVICE_A10:
      global_mem_size_ = 8L * 1024 * 1024 * 1024; // 2 x 4GB = 8GB
      break;
    case OPAE_DEVICE_D5005:
      global_mem_size_ = 32L * 1024 * 1024 * 1024; // 4 x 8GB = 32GB
      break;
    defualt:
      global_mem_size_ = 0;
      break;
  }

  max_mem_alloc_size_ = global_mem_size_;
  mem_blocks_.emplace(0, std::make_pair(global_mem_size_, true));
  free_blocks_by_size_.emplace(global_mem_size_, 0);

  err = fpgaOpen(opae_accelerator_token_, &opae_handle_, 0);
  CHECK_ERROR(err);
  err = fpgaReset(opae_handle_);
  CHECK_ERROR(err);

  opae_buffer_byte_ = 1L * 1024 * 1024 * 1024; // fit to 1GB hugepage
  opae_buffer_line_ = opae_buffer_byte_ / LINE_SIZE;
  err = fpgaPrepareBuffer(opae_handle_, opae_buffer_byte_,
                          (void**)&opae_buffer_ptr_, &opae_buffer_, 0);
  CHECK_ERROR(err);
  err = fpgaGetIOAddress(opae_handle_, opae_buffer_, &opae_buffer_addr_);
  CHECK_ERROR(err);
  SNUCL_INFO("[OPAEDevice] Buffer allocated (size = 0x%zX, virtual address = 0x%zX, physical address = 0x%zX)", opae_buffer_byte_, opae_buffer_ptr_, opae_buffer_addr_);

  device_last_kernel_ = -1;
}

OPAEDevice::~OPAEDevice() {
  fpga_result err = FPGA_OK;
  err = fpgaClose(opae_handle_);
  CHECK_ERROR(err);
  err = fpgaDestroyToken(&opae_accelerator_token_);
  CHECK_ERROR(err);
  err = fpgaDestroyToken(&opae_device_token_);
  CHECK_ERROR(err);
  for (std::map<CLProgram*, CLKernel*>::iterator it = all_kernel_.begin();
       it != all_kernel_.end();
       ++it) {
    if (it->second != NULL) {
      it->second->Release();
    }
  }
}

void OPAEDevice::LaunchKernel(CLCommand* command, CLKernel* kernel,
                                cl_uint work_dim, size_t gwo[3], size_t gws[3],
                                size_t lws[3], size_t nwg[3],
                                map<cl_uint, CLKernelArg*>* kernel_args) {
  SNUCL_INFO("[LaunchKernel] launching %s", kernel->name());

  CLProgram* program = kernel->program();

  unsigned int kernel_id;
  if (all_kernel_[program] != NULL) {
    SNUCL_INFO("[LaunchKernel] using __all__");
    if (device_last_kernel_ != all_kernel_[program]->snucl_index()) {
      if (!PartialReconfig(all_kernel_[program])) {
        return;
      }
      device_last_kernel_ = all_kernel_[program]->snucl_index();
    }
    kernel_id = kernel->snucl_index();
  } else {
    if (device_last_kernel_ != kernel->snucl_index()) {
      if (!PartialReconfig(kernel)) {
        return;
      }
      device_last_kernel_ = kernel->snucl_index();
    }
    kernel_id = 0;
  }

  SetKernelParam(kernel, work_dim, gwo, gws, lws, nwg, kernel_args);
  fpga_result err = FPGA_OK;
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1002 * 4, kernel_id);
  CHECK_ERROR(err);
  BusyWait(0x1004 * 4, 0x8, 0x8, 1);
}

void OPAEDevice::LaunchNativeKernel(CLCommand* command,
                                      void (*user_func)(void*),
                                      void* native_args, size_t size,
                                      cl_uint num_mem_objects, CLMem** mem_list,
                                      ptrdiff_t* mem_offsets) {
}

void OPAEDevice::DMARead(size_t dev_addr, size_t host_addr, size_t num_lines) {
  SNUCL_INFO("[DMARead] Will copy 0x%zX lines from device memory(0x%zX) to buffer(pa=0x%zX)", num_lines, dev_addr, host_addr);
  assert(dev_addr % LINE_SIZE == 0);
  assert(host_addr % LINE_SIZE == 0);
  assert(num_lines <= opae_buffer_line_);
  fpga_result err = FPGA_OK;
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x16 * 4, dev_addr);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x18 * 4, host_addr);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1a * 4, num_lines);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x14 * 4, 1);
  CHECK_ERROR(err);
  BusyWait(0x10 * 4, 0x3, 0x3, 1);
  SNUCL_INFO("[DMARead] Done");
}

void OPAEDevice::DMAWrite(size_t dev_addr, size_t host_addr, size_t num_lines) {
  SNUCL_INFO("[DMAWrite] Will copy 0x%zX lines from buffer(pa=0x%zX) to device memory(0x%zX)", num_lines, host_addr, dev_addr);
  assert(dev_addr % LINE_SIZE == 0);
  assert(host_addr % LINE_SIZE == 0);
  assert(num_lines <= opae_buffer_line_);
  fpga_result err = FPGA_OK;
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x16 * 4, dev_addr);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x18 * 4, host_addr);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1a * 4, num_lines);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x12 * 4, 1);
  CHECK_ERROR(err);
  BusyWait(0x10 * 4, 0x3, 0x3, 1);
  SNUCL_INFO("[DMAWrite] Done");
}

// TODO(heehoon): size == 0?
void OPAEDevice::ReadBufferImpl(size_t dev_addr, void* host_addr, size_t size) {
  size_t first_byte = dev_addr;
  size_t last_byte = first_byte + size - 1;
  size_t first_line = first_byte / LINE_SIZE;
  size_t last_line = last_byte / LINE_SIZE;

  SNUCL_INFO("[ReadBufferImpl] Will copy 0x%zX bytes from device memory(0x%zX) to user memory(0x%zX)", size, first_byte, host_addr);
  while (last_line - first_line + 1 > 0) {
    size_t lines_now = std::min(opae_buffer_line_, last_line - first_line + 1);
    size_t bytes_now = std::min(opae_buffer_byte_ - first_byte % LINE_SIZE, last_byte - first_byte + 1);
    DMARead(first_line * LINE_SIZE, opae_buffer_addr_, lines_now);
    memcpy(host_addr, opae_buffer_ptr_ + first_byte % LINE_SIZE, bytes_now);
    SNUCL_INFO("[ReadBufferImpl] Copied 0x%zX bytes from buffer(va=0x%zX) to user memory(0x%zX)", bytes_now, opae_buffer_ptr_ + first_byte % LINE_SIZE, host_addr);
    first_line += lines_now;
    first_byte += bytes_now;
    host_addr = (char*)host_addr + bytes_now;
  }
  SNUCL_INFO("[ReadBufferImpl] Done");
}

void OPAEDevice::WriteBufferImpl(size_t dev_addr, void* host_addr, size_t size) {
  size_t first_byte = dev_addr;
  size_t last_byte = first_byte + size - 1;
  size_t first_line = first_byte / LINE_SIZE;
  size_t last_line = last_byte / LINE_SIZE;

  SNUCL_INFO("[WriteBufferImpl] Will copy 0x%zX bytes from user memory(0x%zX) to device memory(0x%zX)", size, host_addr, first_byte);
  while (last_line - first_line + 1 > 0) {
    size_t lines_now = std::min(opae_buffer_line_, last_line - first_line + 1);
    size_t bytes_now = std::min(opae_buffer_byte_ - first_byte % LINE_SIZE, last_byte - first_byte + 1);
    if (first_byte % LINE_SIZE != 0) {
      SNUCL_INFO("[WriteBufferImpl] First byte is unaligned (0x%zX %% 0x%zX != 0x%zX)", first_byte, LINE_SIZE, 0);
      DMARead(first_line * LINE_SIZE, opae_buffer_addr_, 1);
    }
    if ((first_byte + bytes_now - 1) % LINE_SIZE != LINE_SIZE - 1) {
      SNUCL_INFO("[WriteBufferImpl] Last byte is unaligned (0x%zX %% 0x%zX != 0x%zX)", first_byte + bytes_now - 1, LINE_SIZE, LINE_SIZE - 1);
      DMARead((first_line + lines_now - 1) * LINE_SIZE, opae_buffer_addr_ + (lines_now - 1) * LINE_SIZE, 1);
    }
    memcpy(opae_buffer_ptr_ + first_byte % LINE_SIZE, host_addr, bytes_now);
    SNUCL_INFO("[WriteBufferImpl] Copied 0x%zX bytes from user memory(0x%zX) to buffer(va=0x%zX)", bytes_now, host_addr, opae_buffer_ptr_ + first_byte % LINE_SIZE);
    DMAWrite(first_line * LINE_SIZE, opae_buffer_addr_, lines_now);
    first_line += lines_now;
    first_byte += bytes_now;
    host_addr = (char*)host_addr + bytes_now;
  }
  SNUCL_INFO("[WriteBufferImpl] Done");
}

void OPAEDevice::ReadBuffer(CLCommand* command, CLMem* mem_src,
                            size_t off_src, size_t size, void* ptr) {
  ReadBufferImpl((size_t)mem_src->GetDevSpecific(this) + off_src, ptr, size);
}

void OPAEDevice::WriteBuffer(CLCommand* command, CLMem* mem_dst,
                               size_t off_dst, size_t size, void* ptr) {
  WriteBufferImpl((size_t)mem_dst->GetDevSpecific(this) + off_dst, ptr, size);
}

void OPAEDevice::CopyBuffer(CLCommand* command, CLMem* mem_src,
                              CLMem* mem_dst, size_t off_src, size_t off_dst,
                              size_t size) {
  void *buf = malloc(size);
  ReadBufferImpl((size_t)mem_src->GetDevSpecific(this) + off_src, buf, size);
  WriteBufferImpl((size_t)mem_dst->GetDevSpecific(this) + off_dst, buf, size);
  free(buf);
}

void OPAEDevice::ReadImage(CLCommand* command, CLMem* mem_src,
                             size_t src_origin[3], size_t region[3],
                             size_t dst_row_pitch, size_t dst_slice_pitch,
                             void* ptr) {
}

void OPAEDevice::WriteImage(CLCommand* command, CLMem* mem_dst,
                              size_t dst_origin[3], size_t region[3],
                              size_t src_row_pitch, size_t src_slice_pitch,
                              void* ptr) {
}

void OPAEDevice::CopyImage(CLCommand* command, CLMem* mem_src, CLMem* mem_dst,
                             size_t src_origin[3], size_t dst_origin[3],
                             size_t region[3]) {
}

void OPAEDevice::CopyImageToBuffer(CLCommand* command, CLMem* mem_src,
                                     CLMem* mem_dst, size_t src_origin[3],
                                     size_t region[3], size_t off_dst) {
}

void OPAEDevice::CopyBufferToImage(CLCommand* command, CLMem* mem_src,
                                     CLMem* mem_dst, size_t off_src,
                                     size_t dst_origin[3], size_t region[3]) {
}

void OPAEDevice::ReadBufferRect(CLCommand* command, CLMem* mem_src,
                                  size_t src_origin[3], size_t dst_origin[3],
                                  size_t region[3], size_t src_row_pitch,
                                  size_t src_slice_pitch, size_t dst_row_pitch,
                                  size_t dst_slice_pitch, void* ptr) {
  void *buf = malloc(mem_src->size());
  ReadBufferImpl((size_t)mem_src->GetDevSpecific(this), buf, mem_src->size());
  CopyRegion(buf, ptr, 3, src_origin, dst_origin,
             region, 1, src_row_pitch, src_slice_pitch, dst_row_pitch,
             dst_slice_pitch);
  free(buf);
}

void OPAEDevice::WriteBufferRect(CLCommand* command, CLMem* mem_dst,
                                   size_t src_origin[3], size_t dst_origin[3],
                                   size_t region[3], size_t src_row_pitch,
                                   size_t src_slice_pitch, size_t dst_row_pitch,
                                   size_t dst_slice_pitch, void* ptr) {
  void *buf = malloc(mem_dst->size());
  ReadBufferImpl((size_t)mem_dst->GetDevSpecific(this), buf, mem_dst->size());
  CopyRegion(ptr, buf, 3, src_origin, dst_origin,
             region, 1, src_row_pitch, src_slice_pitch, dst_row_pitch,
             dst_slice_pitch);
  WriteBufferImpl((size_t)mem_dst->GetDevSpecific(this), buf, mem_dst->size());
  free(buf);
}

void OPAEDevice::CopyBufferRect(CLCommand* command, CLMem* mem_src,
                                  CLMem* mem_dst, size_t src_origin[3],
                                  size_t dst_origin[3], size_t region[3],
                                  size_t src_row_pitch, size_t src_slice_pitch,
                                  size_t dst_row_pitch,
                                  size_t dst_slice_pitch) {
  void *buf_src = malloc(mem_src->size());
  void *buf_dst = malloc(mem_dst->size());
  ReadBufferImpl((size_t)mem_src->GetDevSpecific(this), buf_src, mem_src->size());
  ReadBufferImpl((size_t)mem_dst->GetDevSpecific(this), buf_dst, mem_dst->size());
  CopyRegion(buf_src, buf_dst, 3,
             src_origin, dst_origin, region, 1, src_row_pitch, src_slice_pitch,
             dst_row_pitch, dst_slice_pitch);
  WriteBufferImpl((size_t)mem_dst->GetDevSpecific(this), buf_dst, mem_dst->size());
  free(buf_src);
  free(buf_dst);
}

void OPAEDevice::FillBuffer(CLCommand* command, CLMem* mem_dst, void* pattern,
                              size_t pattern_size, size_t off_dst,
                              size_t size) {
  void *buf = malloc(size);
  size_t index = 0;
  while (index + pattern_size <= size) {
    memcpy((char*)buf + index, pattern, pattern_size);
    index += pattern_size;
  }
  WriteBufferImpl((size_t)mem_dst->GetDevSpecific(this) + off_dst, buf, mem_dst->size());
}

void OPAEDevice::FillImage(CLCommand* command, CLMem* mem_dst,
                             void* fill_color, size_t dst_origin[3],
                             size_t region[3]) {
}

void OPAEDevice::BuildProgram(CLCommand* command, CLProgram* program,
                                CLProgramSource* source,
                                CLProgramBinary* binary, const char* options) {
  if (binary != NULL && binary->type() == CL_PROGRAM_BINARY_TYPE_EXECUTABLE) {
    void* executable = LoadBinary(program, binary->core());
    program->CompleteBuild(this, CL_BUILD_SUCCESS, NULL, binary, executable);

    cl_int err;
    all_kernel_[program] = program->CreateKernel("__all__", &err);
    if (all_kernel_[program] != NULL) {
      SNUCL_INFO("[BuildProgram] trying PR with __all__ kernel...");
      if (PartialReconfig(all_kernel_[program])) {
        SNUCL_INFO("[BuildProgram] PR with __all__ kernel was successful");
        device_last_kernel_ = all_kernel_[program]->snucl_index();
      } else {
        SNUCL_INFO("[BuildProgram] PR with __all__ kernel failed");
      }
    } else {
      CLKernel* first_kernel = program->CreateKernel(kernel_names_[0].c_str(), &err);
      if (first_kernel != NULL) {
        SNUCL_INFO("[BuildProgram] trying PR with the first kernel...");
        if (PartialReconfig(first_kernel)) {
          SNUCL_INFO("[BuildProgram] PR with the first kernel was successful");
          device_last_kernel_ = first_kernel->snucl_index();
        } else {
          SNUCL_INFO("[BuildProgram] PR with the first kernel failed");
        }
      }
      delete first_kernel;
    }
  }
}

void OPAEDevice::CompileProgram(CLCommand* command, CLProgram* program,
                               CLProgramSource* source, const char* options,
                               size_t num_headers, CLProgramSource** headers) {
  const char* message = "Online compilation is not supported";
  char* build_log = (char*)malloc(strlen(message) + 1);
  strcpy(build_log, message);
  program->CompleteBuild(this, CL_BUILD_ERROR, build_log, NULL);
}

void OPAEDevice::LinkProgram(CLCommand* command, CLProgram* program,
                            size_t num_binaries, CLProgramBinary** binaries,
                            const char* options) {
  const char* message = "Online compilation is not supported";
  char* build_log = (char*)malloc(strlen(message) + 1);
  strcpy(build_log, message);
  program->CompleteBuild(this, CL_BUILD_ERROR, build_log, NULL);
}

void* OPAEDevice::AllocMem(CLMem* mem) {
  SNUCL_INFO("[AllocMem] malloc(%zX) requested", mem->size());

  // Find a free block which the request fits
  size_t aligned_size = (mem->size() + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
  auto it = free_blocks_by_size_.lower_bound(std::make_pair(aligned_size, 0));
  if (it == free_blocks_by_size_.end()) {
    SNUCL_ERROR_EXIT("[AllocMem] Cannot allocate %zX bytes", aligned_size);
  }

  // Remove the free block
  size_t free_block_size = it->first;
  size_t free_block_addr = it->second;
  free_blocks_by_size_.erase(it);
  mem_blocks_.erase(free_block_addr);
  SNUCL_INFO("[AllocMem] Remove free block (addr=%zX, size=%zX)", free_block_addr, free_block_size);

  // If the free block is bigger than requested size, segment it
  if (free_block_size > aligned_size) {
    size_t new_size = free_block_size - aligned_size;
    size_t new_addr = free_block_addr + aligned_size;
    free_blocks_by_size_.emplace(new_size, new_addr);
    mem_blocks_.emplace(new_addr, std::make_pair(new_size, true));
    SNUCL_INFO("[AllocMem] Add free block (addr=%zX, size=%zX)", new_addr, new_size);
  }

  // Insert the allocated block
  mem_blocks_.emplace(free_block_addr, std::make_pair(aligned_size, false));
  SNUCL_INFO("[AllocMem] Add used block (addr=%zX, size=%zX)", free_block_addr, aligned_size);

  return (void*)free_block_addr;
}

void OPAEDevice::FreeMem(CLMem* mem, void* dev_specific) {
  size_t addr = (size_t)mem->GetDevSpecific(this);
  SNUCL_INFO("[FreeMem] free(%zX) requested", addr);

  auto it = mem_blocks_.find(addr);
  assert(it != mem_blocks_.end() && !it->second.second);

  size_t new_size = it->second.first;
  size_t new_addr = it->first;
  SNUCL_INFO("[FreeMem] Remove used block (addr=%zX, size=%zX)", new_addr, new_size);

  if (it != mem_blocks_.begin()) {
    auto prev = std::prev(it);
    if (prev->second.second) { // prev block is free => merge
      size_t prev_size = prev->second.first;
      size_t prev_addr = prev->first;
      free_blocks_by_size_.erase(std::make_pair(prev_size, prev_addr));
      mem_blocks_.erase(prev);
      SNUCL_INFO("[FreeMem] Remove free block (addr=%zX, size=%zX)", prev_addr, prev_size);
      new_size += prev_size;
      new_addr = prev_addr;
    }
  }

  auto next = std::next(it);
  if (next != mem_blocks_.end()) {
    if (next->second.second) { // next block is free => merge
      size_t next_size = next->second.first;
      size_t next_addr = next->first;
      free_blocks_by_size_.erase(std::make_pair(next_size, next_addr));
      mem_blocks_.erase(next);
      SNUCL_INFO("[FreeMem] Remove free block (addr=%zX, size=%zX)", next_addr, next_size);
      new_size += next_size;
    }
  }

  // Remove the allocated block
  mem_blocks_.erase(it);

  // Insert the free block
  free_blocks_by_size_.emplace(new_size, new_addr);
  mem_blocks_.emplace(new_addr, std::make_pair(new_size, true));
  SNUCL_INFO("[FreeMem] Add free block (addr=%zX, size=%zX)", new_addr, new_size);
}

void OPAEDevice::FreeExecutable(CLProgram* program, void* executable) {
  delete [] ((OPAEBitstream*)executable);
}

void* OPAEDevice::AllocKernel(CLKernel* kernel) {
  CLProgram* program = kernel->program();
  OPAEBitstream* bitstreams = (OPAEBitstream*)program->GetExecutable(this);
  int kernel_index = kernel->snucl_index();
  return &bitstreams[kernel_index];
}

void OPAEDevice::FreeKernel(CLKernel* kernel, void* dev_specific) {
  // Do nothing
}

unsigned int OPAEDevice::BusyWait(uint64_t mmio_addr, unsigned int mask,
                                  unsigned int wait_value,
                                  unsigned int interval) {
  while (true) {
    uint64_t ret;
    fpgaReadMMIO64(opae_handle_, 0, mmio_addr, &ret);
    if ((ret & mask) == wait_value) return ret;
    usleep(interval);
  }
}

bool OPAEDevice::PartialReconfig(CLKernel* kernel) {
#ifndef USE_ASE
  SNUCL_INFO("[PartialReconfig] start");
  OPAEBitstream* bitstream = (OPAEBitstream*)kernel->GetDevSpecific(this);
  fpga_result err = FPGA_OK;
  err = fpgaClose(opae_handle_);
  CHECK_ERROR(err);
  {
    fpga_handle opae_device_handle;
    err = fpgaOpen(opae_device_token_, &opae_device_handle, 0);
    CHECK_ERROR(err);
    err = fpgaReconfigureSlot(opae_device_handle, 0, bitstream->data, bitstream->size, 0);
    CHECK_ERROR(err);
    err = fpgaClose(opae_device_handle);
    CHECK_ERROR(err);
  }
  err = fpgaOpen(opae_accelerator_token_, &opae_handle_, 0);
  CHECK_ERROR(err);
  err = fpgaReset(opae_handle_);
  CHECK_ERROR(err);
  SNUCL_INFO("[PartialReconfig] end");
#endif
  return true;
}

void OPAEDevice::SetKernelParam(CLKernel* kernel, cl_uint work_dim,
                                  size_t gwo[3], size_t gws[3], size_t lws[3],
                                  size_t nwg[3],
                                  map<cl_uint, CLKernelArg*>* kernel_args) {
  SNUCL_INFO("[SetKernelParam] work_dim = %d", work_dim);
  SNUCL_INFO("[SetKernelParam] gwo = {%d, %d, %d}", gwo[0], gwo[1], gwo[2]);
  SNUCL_INFO("[SetKernelParam] gws = {%d, %d, %d}", gws[0], gws[1], gws[2]);
  SNUCL_INFO("[SetKernelParam] lws = {%d, %d, %d}", lws[0], lws[1], lws[2]);
  SNUCL_INFO("[SetKernelParam] nwg = {%d, %d, %d}", nwg[0], nwg[1], nwg[2]);

  char args[4096];
  unsigned int args_size = 0;
  uint64_t local_ofs = 0;

  for (map<cl_uint, CLKernelArg*>::iterator it = kernel_args->begin();
       it != kernel_args->end();
       ++it) {
    CLMem* mem = it->second->mem;
    CLSampler* sampler = it->second->sampler;
    if (mem != NULL) {
      void* ptr = mem->GetDevSpecific(this);
      if (args_size + sizeof(ptr) > 4096) {
        SNUCL_ERROR_EXIT("argument size out of range");
      }
      memcpy(args + args_size, (void*)&ptr, sizeof(ptr));
      args_size += sizeof(ptr);
    } else if (sampler != NULL) {
      // Do nothing
    } else if (it->second->local) {
      uint64_t local_size = it->second->size;
      memcpy(args + args_size, &local_ofs, sizeof(local_ofs));
      if (args_size + sizeof(local_ofs) > 4096) {
        SNUCL_ERROR_EXIT("argument size out of range");
      }
      args_size += sizeof(local_ofs);
      SNUCL_INFO("[SetKernelParam] local memory alloc (ofs = %d, size = %d)", local_ofs, local_size);
      local_ofs += local_size;
      local_ofs = (local_ofs + 3) / 4 * 4;
    } else {
      memcpy(args + args_size, it->second->value, it->second->size);
      if (args_size + it->second->size > 4096) {
        SNUCL_ERROR_EXIT("argument size out of range");
      }
      args_size += it->second->size;
    }
  }

  fpga_result err = FPGA_OK;
  uint64_t mmio_addr = 0x1100 * 4;
  for (unsigned int i = 0; i < args_size; i += 8) {
    SNUCL_INFO("[SetKernelParam] *%016lX = %016lX", mmio_addr, *((uint64_t*)(args + i)));
    err = fpgaWriteMMIO64(opae_handle_, 0, mmio_addr, *((uint64_t*)(args + i)));
    CHECK_ERROR(err);
    mmio_addr += 8;
  }

  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1010 * 4, gws[0]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1012 * 4, gws[1]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1014 * 4, gws[2]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1020 * 4, lws[0]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1022 * 4, lws[1]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1024 * 4, lws[2]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1030 * 4, nwg[0]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1032 * 4, nwg[1]);
  CHECK_ERROR(err);
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1034 * 4, nwg[2]);
  CHECK_ERROR(err);
  size_t num_items = gws[0] * gws[1] * gws[2];
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1040 * 4, num_items);
  CHECK_ERROR(err);
  size_t num_groups = nwg[0] * nwg[1] * nwg[2];
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1042 * 4, num_groups);
  CHECK_ERROR(err);
  size_t wg_size = lws[0] * lws[1] * lws[2];
  err = fpgaWriteMMIO64(opae_handle_, 0, 0x1044 * 4, wg_size);
  CHECK_ERROR(err);
}

void* OPAEDevice::LoadBinary(CLProgram* program,
                             const unsigned char* raw_binary) {
  if (!program->BeginRegisterKernelInfo())
    return NULL;

  size_t num_kernels;
  const size_t* kernel_offsets;
  memcpy(&num_kernels, raw_binary, sizeof(size_t));
  kernel_offsets = (const size_t*)(raw_binary + 8);

  OPAEBitstream* bitstreams = new OPAEBitstream[num_kernels];

  for (int kernel_index = 0; kernel_index < num_kernels; kernel_index++) {
    const unsigned char* kernel_binary =
        raw_binary + kernel_offsets[kernel_index];

    unsigned int frequency;
    cl_uint num_args;
    size_t work_group_size;
    size_t compile_work_group_size[3];
    cl_ulong local_mem_size;
    size_t preferred_work_group_size_multiple;
    cl_ulong private_mem_size;
    int snucl_index;
    char name[256 + 1];
    char attributes[512 + 1];

    memcpy(&frequency, kernel_binary + 8, sizeof(frequency));
    memcpy(&num_args, kernel_binary + 12, sizeof(num_args));
    memcpy(&work_group_size, kernel_binary + 16, sizeof(work_group_size));
    memcpy(compile_work_group_size, kernel_binary + 24,
           sizeof(compile_work_group_size));
    memcpy(&local_mem_size, kernel_binary + 48, sizeof(local_mem_size));
    memcpy(&preferred_work_group_size_multiple, kernel_binary + 56,
           sizeof(preferred_work_group_size_multiple));
    memcpy(&private_mem_size, kernel_binary + 64, sizeof(private_mem_size));
    memcpy(&snucl_index, kernel_binary + 72, sizeof(snucl_index));
    memcpy(name, kernel_binary + 256, 256); name[256] = '\0';
    memcpy(attributes, kernel_binary + 512, 512); name[512] = '\0';
    if (strcmp(name, "__all__") == 0) snucl_index = num_kernels - 1;
    kernel_names_[snucl_index] = name;
    program->RegisterKernelInfo(name, num_args, attributes, this,
                                work_group_size, compile_work_group_size,
                                local_mem_size,
                                preferred_work_group_size_multiple,
                                private_mem_size, snucl_index);
    kernel_binary += 1024;

    for (cl_uint arg_index = 0; arg_index < num_args; arg_index++) {
      char arg_name[64 + 1];
      cl_kernel_arg_address_qualifier address_qualifier;
      cl_kernel_arg_access_qualifier access_qualifier;
      char type_name[176 + 1];
      cl_kernel_arg_type_qualifier type_qualifier;

      memcpy(arg_name, kernel_binary, 64); arg_name[64] = '\0';
      memcpy(&address_qualifier, kernel_binary + 64, sizeof(address_qualifier));
      memcpy(&access_qualifier, kernel_binary + 68, sizeof(access_qualifier));
      memcpy(type_name, kernel_binary + 72, 176); type_name[176] = '\0';
      memcpy(&type_qualifier, kernel_binary + 248, sizeof(type_qualifier));
      program->RegisterKernelArgInfo(name, arg_index, address_qualifier,
                                     access_qualifier, type_name,
                                     type_qualifier, arg_name);
      kernel_binary += 256;
    }

    size_t bitstream_size;
    memcpy(&bitstream_size, kernel_binary, sizeof(size_t));
    kernel_binary += 8;

    bitstreams[snucl_index].data = kernel_binary;
    bitstreams[snucl_index].size = bitstream_size;
    bitstreams[snucl_index].frequency = frequency;
  }

  program->FinishRegisterKernelInfo();

  return bitstreams;
}

void OPAEDevice::CreateDevices() {
  fpga_result err;

  fpga_properties filter = NULL;
  err = fpgaGetProperties(NULL, &filter);
  CHECK_ERROR(err);

  err = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
  CHECK_ERROR(err);
  uint32_t num_dev;
  err = fpgaEnumerate(&filter, 1, NULL, 0, &num_dev);
  CHECK_ERROR(err);
  std::vector<fpga_token> dev_tokens(num_dev);
  err = fpgaEnumerate(&filter, 1, dev_tokens.data(), num_dev, &num_dev);
  CHECK_ERROR(err);

  err = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
  CHECK_ERROR(err);
  uint32_t num_acc;
  err = fpgaEnumerate(&filter, 1, NULL, 0, &num_acc);
  CHECK_ERROR(err);
  std::vector<fpga_token> acc_tokens(num_acc);
  err = fpgaEnumerate(&filter, 1, acc_tokens.data(), num_acc, &num_acc);
  CHECK_ERROR(err);

  SNUCL_INFO("# of devices = %d, # of accelerators = %d", num_dev, num_acc);
  if (num_dev != num_acc) {
    SNUCL_ERROR_EXIT("# of devices and accelerators mismatch.");
  }

  for (int i = 0; i < num_acc; ++i) {
    auto acc_token = acc_tokens[i];
    err = fpgaUpdateProperties(acc_token, filter);
    CHECK_ERROR(err);
    uint16_t device_id;
    err = fpgaPropertiesGetDeviceID(filter, &device_id);
    CHECK_ERROR(err);
    OPAEDevice* device;
    switch (device_id) {
      case 0x09C4:
        // Intel PAC A10
        device = new OPAEDevice(dev_tokens[i], acc_tokens[i], OPAE_DEVICE_A10);
        break;
      case 0x0B2B:
        // Intel PAC D5005
        device = new OPAEDevice(dev_tokens[i], acc_tokens[i], OPAE_DEVICE_D5005);
        break;
      default:
        SNUCL_INFO("Device(id=0x%04X) not supported", device_id);
    }
  }

  err = fpgaDestroyProperties(&filter);
  CHECK_ERROR(err);
}
