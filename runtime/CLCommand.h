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

#ifndef __SNUCL__CL_COMMAND_H
#define __SNUCL__CL_COMMAND_H

#include <map>
#include <vector>
#include <CL/cl.h>
#include <CL/cl_ext_snucl.h>
#include "CLKernel.h"
#include "Structs.h"

#define CL_COMMAND_BUILD_PROGRAM   0x1210
#define CL_COMMAND_COMPILE_PROGRAM 0x1211
#define CL_COMMAND_LINK_PROGRAM    0x1212
#define CL_COMMAND_WAIT_FOR_EVENTS 0x1213
#define CL_COMMAND_CUSTOM          0x1214
#define CL_COMMAND_NOP             0x1215

class CLCommandQueue;
class CLContext;
class CLDevice;
class CLEvent;
class CLFile;
class CLMem;
class CLProgram;
class CLProgramBinary;
class CLProgramSource;

class CLCommand {
 public:
  CLCommand(CLContext* context, CLDevice* device, CLCommandQueue* queue,
            cl_command_type type);
  ~CLCommand();

  cl_command_type type() const { return type_; }
  CLContext* context() const { return context_; }
  CLDevice* device() const { return device_; }

  CLDevice* source_device() const { return dev_src_; }
  CLDevice* destination_device() const { return dev_dst_; }
  int source_node() const { return node_src_; }
  int destination_node() const { return node_dst_; }
  unsigned long event_id() const { return event_id_; }

  void SetWaitList(cl_uint num_events_in_wait_list,
                   const cl_event* event_wait_list);
  void AddWaitEvent(CLEvent* event);
  bool IsExecutable();

  void Submit();
  void SetError(cl_int error);
  void SetAsRunning();
  void SetAsComplete();

  CLEvent* ExportEvent();

  /* Extra annotations for commands
   * 
   * SourceDevice/DestinationDevice
   *   used for CopyBuffer, CopyImage, CopyImageToBuffer, CopyBufferToImage,
   *            and CopyBufferRect
   *
   * SourceNode
   *   used for WriteBuffer, WriteImage, and WriteBufferRect
   *
   * DestinationNode
   *   used for ReadBuffer, ReadImage, and ReadBufferRect
   *
   * SourceDevice
   *   used for Broadcast
   */
  void AnnotateSourceDevice(CLDevice* device);
  void AnnotateDestinationDevice(CLDevice* device);
  void AnnotateSourceNode(int node);
  void AnnotateDestinationNode(int node);

  bool IsPartialCommand() const;
  void SetAsPartialCommand(CLCommand* root);

  void Execute();
  bool ResolveConsistency();

 private:
  bool ResolveConsistencyOfLaunchKernel();
  bool ResolveConsistencyOfLaunchNativeKernel();
  bool ResolveConsistencyOfReadMem();
  bool ResolveConsistencyOfWriteMem();
  bool ResolveConsistencyOfCopyMem();
  bool ResolveConsistencyOfMap();
  bool ResolveConsistencyOfUnmap();
  bool ResolveConsistencyOfBroadcast();
  bool ResolveConsistencyOfAlltoAll();
  bool ResolveConsistencyOfCopyMemToFile();
  bool ResolveConsistencyOfCopyFileToMem();

  void UpdateConsistencyOfLaunchKernel();
  void UpdateConsistencyOfLaunchNativeKernel();
  void UpdateConsistencyOfReadMem();
  void UpdateConsistencyOfWriteMem();
  void UpdateConsistencyOfCopyMem();
  void UpdateConsistencyOfMap();
  void UpdateConsistencyOfUnmap();
  void UpdateConsistencyOfBroadcast();
  void UpdateConsistencyOfAlltoAll();
  void UpdateConsistencyOfCopyMemToFile();
  void UpdateConsistencyOfCopyFileToMem();

  void GetCopyPattern(CLDevice* dev_src, CLDevice* dev_dst, bool& use_read,
                      bool& use_write, bool& use_copy, bool& use_send,
                      bool& use_recv, bool& use_rcopy, bool& alloc_ptr,
                      bool& use_host_ptr);
  CLEvent* CloneMem(CLDevice* dev_src, CLDevice* dev_dst, CLMem* mem);

  bool LocateMemOnDevice(CLMem* mem);
  void AccessMemOnDevice(CLMem* mem, bool write);
  bool ChangeDeviceToReadMem(CLMem* mem, CLDevice*& device);

  cl_command_type type_;
  CLCommandQueue* queue_;
  CLContext* context_;
  CLDevice* device_;
  CLEvent* event_;
  std::vector<CLEvent*> wait_events_;
  bool wait_events_complete_;
  bool wait_events_good_;
  bool consistency_resolved_;
  cl_int error_;

  CLDevice* dev_src_;
  CLDevice* dev_dst_;
  int node_src_;
  int node_dst_;
  size_t event_id_;

  CLMem* mem_src_;
  CLMem* mem_dst_;
  size_t off_src_;
  size_t off_dst_;
  size_t size_;
  void* ptr_;

  void* pattern_;
  size_t pattern_size_;

  CLKernel* kernel_;
  cl_uint work_dim_;
  size_t gwo_[3];
  size_t gws_[3];
  size_t lws_[3];
  size_t nwg_[3];
  std::map<cl_uint, CLKernelArg*>* kernel_args_;

  void (*user_func_)(void*);
  void* native_args_;
  cl_uint num_mem_objects_;
  CLMem** mem_list_;
  ptrdiff_t* mem_offsets_;

  size_t src_origin_[3];
  size_t dst_origin_[3];
  size_t region_[3];
  size_t src_row_pitch_;
  size_t src_slice_pitch_;
  size_t dst_row_pitch_;
  size_t dst_slice_pitch_;

  void* temp_buf_;

  cl_map_flags map_flags_;
  cl_mem_migration_flags migration_flags_;

  CLProgram* program_;
  CLProgramSource* source_;
  CLProgramBinary* binary_;
  const char* options_;
  CLProgramSource** headers_;
  CLProgramBinary** link_binaries_;

  CLFile* file_src_;
  CLFile* file_dst_;
  const char* filename_;
  cl_file_open_flags file_open_flags_;

  void (*custom_function_)(void*);
  void* custom_data_;

 public:
  static CLCommand*
  CreateReadBuffer(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                   CLMem* buffer, size_t offset, size_t size, void* ptr);

  static CLCommand*
  CreateReadBufferRect(CLContext* context, CLDevice* device,
                       CLCommandQueue* queue, CLMem* buffer,
                       const size_t* buffer_origin, const size_t* host_origin,
                       const size_t* region, size_t buffer_row_pitch,
                       size_t buffer_slice_pitch, size_t host_row_pitch,
                       size_t host_slice_pitch, void* ptr);

  static CLCommand*
  CreateWriteBuffer(CLContext* context, CLDevice* device,
                    CLCommandQueue* queue, CLMem* buffer, size_t offset,
                    size_t size, void* ptr);

  static CLCommand*
  CreateWriteBufferRect(CLContext* context, CLDevice* device,
                        CLCommandQueue* queue, CLMem* buffer,
                        const size_t* buffer_origin, const size_t* host_origin,
                        const size_t* region, size_t buffer_row_pitch,
                        size_t buffer_slice_pitch, size_t host_row_pitch,
                        size_t host_slice_pitch, void* ptr);

  static CLCommand*
  CreateFillBuffer(CLContext* contxt, CLDevice* device, CLCommandQueue* queue,
                   CLMem* buffer, const void* pattern, size_t pattern_size,
                   size_t offset, size_t size);

  static CLCommand*
  CreateCopyBuffer(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                   CLMem* src_buffer, CLMem* dst_buffer, size_t src_offset,
                   size_t dst_offset, size_t size);

  static CLCommand*
  CreateCopyBufferRect(CLContext* context, CLDevice* device,
                       CLCommandQueue* queue, CLMem* src_buffer,
                       CLMem* dst_buffer, const size_t* src_origin,
                       const size_t* dst_origin, const size_t* region,
                       size_t src_row_pitch, size_t src_slice_pitch,
                       size_t dst_row_pitch, size_t dst_slice_pitch);

  static CLCommand*
  CreateReadImage(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                  CLMem* image, const size_t* origin, const size_t* region,
                  size_t row_pitch, size_t slice_pitch, void* ptr);

  static CLCommand*
  CreateWriteImage(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                   CLMem* image, const size_t* origin, const size_t* region,
                   size_t row_pitch, size_t slice_pitch, void* ptr);

  static CLCommand*
  CreateFillImage(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                  CLMem* image, const void* fill_color, const size_t* origin,
                  const size_t* region);

  static CLCommand*
  CreateCopyImage(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                  CLMem* src_image, CLMem* dst_image, const size_t* src_origin,
                  const size_t* dst_origin, const size_t* region);

  static CLCommand*
  CreateCopyImageToBuffer(CLContext* context, CLDevice* device,
                          CLCommandQueue* queue, CLMem* src_image,
                          CLMem* dst_buffer, const size_t* src_origin,
                          const size_t* region, size_t dst_offset);

  static CLCommand*
  CreateCopyBufferToImage(CLContext* context, CLDevice* device,
                          CLCommandQueue* queue, CLMem* src_buffer,
                          CLMem* dst_image, size_t src_offset,
                          const size_t* dst_origin, const size_t* region);

  static CLCommand*
  CreateMapBuffer(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                  CLMem* buffer, cl_map_flags map_flags, size_t offset,
                  size_t size, void* mapped_ptr);

  static CLCommand*
  CreateMapImage(CLContext* context, CLDevice* device, CLCommandQueue* queue,
                 CLMem* image, cl_map_flags map_flags, const size_t* origin,
                 const size_t* region, void* mapped_ptr);

  static CLCommand*
  CreateUnmapMemObject(CLContext* context, CLDevice* device,
                       CLCommandQueue* queue, CLMem* mem, void* mapped_ptr);

  static CLCommand*
  CreateMigrateMemObjects(CLContext* context, CLDevice* device,
                          CLCommandQueue* queue, cl_uint num_mem_objects,
                          const cl_mem* mem_list,
                          cl_mem_migration_flags flags);

  static CLCommand*
  CreateNDRangeKernel(CLContext* context, CLDevice* device,
                      CLCommandQueue* queue, CLKernel* kernel,
                      cl_uint work_dim, const size_t* global_work_offset,
                      const size_t* global_work_size,
                      const size_t* local_work_size);

  static CLCommand*
  CreateNativeKernel(CLContext* context, CLDevice* device,
                     CLCommandQueue* queue, void (*user_func)(void*),
                     void* args, size_t cb_args, cl_uint num_mem_objects,
                     const cl_mem* mem_list, const void** args_mem_loc);

  static CLCommand*
  CreateMarker(CLContext* context, CLDevice* device, CLCommandQueue* queue);

  static CLCommand*
  CreateBarrier(CLContext* context, CLDevice* device, CLCommandQueue* queue);

  static CLCommand*
  CreateWaitForEvents(CLContext* context, CLDevice* device,
                      CLCommandQueue* queue);

  static CLCommand*
  CreateBuildProgram(CLDevice* device, CLProgram* program,
                     CLProgramSource* source, CLProgramBinary* binary,
                     const char* options);

  static CLCommand*
  CreateCompileProgram(CLDevice* device, CLProgram* program,
                       CLProgramSource* source, const char* options,
                       std::vector<CLProgramSource*>& headers);

  static CLCommand*
  CreateLinkProgram(CLDevice* device, CLProgram* program,
                    std::vector<CLProgramBinary*>& binaries,
                    const char* options);

  static CLCommand*
  CreateCustom(CLContext* context, CLDevice* device, CLCommandQueue* queue,
               void (*custom_function)(void*), void* custom_data);

  static CLCommand*
  CreateNop(CLContext* context, CLDevice* device, CLCommandQueue* queue);

  static CLCommand*
  CreateBroadcastBuffer(CLContext* context, CLDevice* device,
                        CLCommandQueue* queue, CLMem* src_buffer,
                        CLMem* dst_buffer, size_t src_offset,
                        size_t dst_offset, size_t cb);

  static CLCommand*
  CreateAlltoAllBuffer(CLContext* context, CLDevice* device,
                       CLCommandQueue* queue, CLMem* src_buffer,
                       CLMem* dst_buffer, size_t src_offset, size_t dst_offset,
                       size_t cb);

  static CLCommand*
  CreateLocalFileOpen(CLContext* context, CLDevice* device,
                      CLCommandQueue* queue, CLFile* file);

  static CLCommand*
  CreateCopyBufferToFile(CLContext* context, CLDevice* device,
                         CLCommandQueue* queue, CLMem* src_buffer,
                         CLFile* dst_file, size_t src_offset,
                         size_t dst_offset, size_t size);

  static CLCommand*
  CreateCopyFileToBuffer(CLContext* context, CLDevice* device,
                         CLCommandQueue* queue, CLFile* src_file,
                         CLMem* dst_buffer, size_t src_offset,
                         size_t dst_offset, size_t size);
};

#endif // __SNUCL__CL_COMMAND_H
