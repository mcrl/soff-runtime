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

#ifndef __SNUCL__CL_PROGRAM_H
#define __SNUCL__CL_PROGRAM_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <pthread.h>
#include <CL/cl.h>
#include "CLObject.h"
#include "Structs.h"

class CLContext;
class CLDevice;
class CLKernel;
class ProgramCallback;

class CLProgramSource {
 public:
  CLProgramSource() {}
  CLProgramSource(CLProgramSource* original);
  ~CLProgramSource() {}

  const char* concat_source() const { return concat_source_.c_str(); }
  size_t concat_source_length() const { return concat_source_.length(); }
  const char* header_name() const { return header_name_.c_str(); }

  void AddSource(const char* source); // null-terminated
  void AddSource(const char* source, size_t length); // fixed-length
  void SetHeaderName(const char* header_name);

 private:
  std::vector<std::string> sources_;
  std::string concat_source_;
  std::string header_name_;
};

class CLProgramBinary {
 public:
  /*
   * The structure of binaries:
   * Bin (type) (hash) (size) (binary for the target device)
   *  3    1      4      8                 size
   * <------- header -------> <------- core_binary -------->
   * <-------------------- full_binary -------------------->
   */
  CLProgramBinary(CLDevice* device, const unsigned char* full_binary,
                  size_t size);
  CLProgramBinary(CLDevice* device, cl_program_binary_type type,
                  const unsigned char* core_binary, size_t size);
  CLProgramBinary(CLProgramBinary* original);
  ~CLProgramBinary();

  CLDevice* device() const { return device_; }
  cl_program_binary_type type() const { return type_; }
  const unsigned char* full() const { return binary_; }
  const unsigned char* core() const { return binary_ + 16; }
  size_t full_size() const { return size_; }
  size_t core_size() const { return size_ - 16; }

  bool IsValid() const { return type_ != CL_PROGRAM_BINARY_TYPE_NONE; }

 private:
  unsigned int Hash(const unsigned char* str, size_t size);

  cl_program_binary_type ParseHeader(const unsigned char* full_binary,
                                     size_t size);
  void GenerateHeader(unsigned char* header, cl_program_binary_type type,
                      const unsigned char* core_binary, size_t size);

  CLDevice* device_;
  cl_program_binary_type type_;
  unsigned char* binary_;
  size_t size_;
};

class CLKernelInfo {
 public:
  CLKernelInfo(const char* name, cl_uint num_args, const char* attributes);
  ~CLKernelInfo();

  const char* name() const { return name_; }
  cl_uint num_args() const { return num_args_; }
  const char* attributes() const { return attributes_; }
  int snucl_index() const { return snucl_index_; }

  cl_int GetKernelWorkGroupInfo(CLDevice* device,
                                cl_kernel_work_group_info param_name,
                                size_t param_value_size, void* param_value,
                                size_t* param_value_size_ret);
  cl_int GetKernelArgInfo(cl_uint arg_index, cl_kernel_arg_info param_name,
                          size_t param_value_size, void* param_value,
                          size_t* param_value_size_ret);

  bool IsValid() const { return valid_; }

  void Update(const char* name, cl_uint num_args, const char* attributes);
  void SetWorkGroupInfo(CLDevice* device, size_t work_group_size,
                        size_t compile_work_group_size[3],
                        cl_ulong local_mem_size,
                        size_t preferred_work_group_size_multiple,
                        cl_ulong private_mem_size);
  void SetArgInfo(cl_uint arg_index,
                  cl_kernel_arg_address_qualifier arg_address_qualifier,
                  cl_kernel_arg_access_qualifier arg_access_qualifier,
                  const char* arg_type_name,
                  cl_kernel_arg_type_qualifier arg_type_qualifier,
                  const char* arg_name);
  void SetSnuCLIndex(int snucl_index);
  void Invalidate();

  size_t GetSerializationSize(CLDevice* device);
  void* SerializeKernelInfo(void* buffer, CLDevice* device);
  // Deserialization is implemented in CLProgram

 private:
  char* name_;
  cl_uint num_args_;
  char* attributes_;

  bool valid_;
  std::set<CLDevice*> devices_;
  std::set<cl_uint> indexes_;

  std::map<CLDevice*, size_t> work_group_size_;
  size_t compile_work_group_size_[3];
  std::map<CLDevice*, cl_ulong> local_mem_size_;
  std::map<CLDevice*, size_t> preferred_work_group_size_multiple_;
  std::map<CLDevice*, cl_ulong> private_mem_size_;

  cl_kernel_arg_address_qualifier* arg_address_qualifiers_;
  cl_kernel_arg_access_qualifier* arg_access_qualifiers_;
  char** arg_type_names_;
  cl_kernel_arg_type_qualifier* arg_type_qualifiers_;
  char** arg_names_;

  int snucl_index_;
};

class CLProgram: public CLObject<struct _cl_program, CLProgram> {
 public:
  CLProgram(CLContext* context, const std::vector<CLDevice*>& devices);
  virtual void Cleanup();
  virtual ~CLProgram();

  CLContext* context() const { return context_; }

  cl_int GetProgramInfo(cl_program_info param_name, size_t param_value_size,
                        void* param_value, size_t* param_value_size_ret);
  cl_int GetProgramBuildInfo(CLDevice* device,
                             cl_program_build_info param_name,
                             size_t param_value_size, void* param_value,
                             size_t* param_value_size_ret);

  cl_int BuildProgram(cl_uint num_devices, const cl_device_id* device_list,
                      const char* options, ProgramCallback* callback);
  cl_int CompileProgram(cl_uint num_devices, const cl_device_id* device_list,
                        const char* options, cl_uint num_input_headers,
                        const cl_program* input_headers,
                        const char** header_include_names,
                        ProgramCallback* callback);
  cl_int LinkProgram(const char* options, cl_uint num_input_programs,
                     const cl_program* input_programs,
                     ProgramCallback* callback);

  CLKernel* CreateKernel(const char* kernel_name, cl_int* err);
  cl_int CreateKernelsInProgram(cl_uint num_kernels, cl_kernel* kernels,
                                cl_uint* num_kernels_ret);

  bool IsValidDevice(CLDevice* device) const;
  bool IsValidDevices(cl_uint num_devices,
                      const cl_device_id* device_list) const;

  bool HasSource() const;
  bool HasValidBinary(CLDevice* device);
  bool HasOneExecutable() const;
  bool HasAllExecutable() const;

  bool IsCompilerAvailable();
  bool IsLinkerAvailable();

  bool EnterBuild(CLDevice* target_device);
  bool EnterBuild(const std::vector<CLDevice*>& target_devices);
  // clBuildProgram, clLinkProgram
  void CompleteBuild(CLDevice* device, cl_build_status status, char* log,
                     CLProgramBinary* binary, void* executable);
  // clCompileProgram
  void CompleteBuild(CLDevice* device, cl_build_status status, char* log,
                     CLProgramBinary* binary);

  bool BeginRegisterKernelInfo();
  void RegisterKernelInfo(const char* name, cl_uint num_args,
                          const char* attributes, CLDevice* device,
                          size_t work_group_size,
                          size_t compile_work_group_size[3],
                          cl_ulong local_mem_size,
                          size_t preferred_work_group_size_multiple,
                          cl_ulong private_mem_size, int snucl_index);
  void RegisterKernelArgInfo(
      const char* name, cl_uint arg_index,
      cl_kernel_arg_address_qualifier arg_address_qualifier,
      cl_kernel_arg_access_qualifier arg_access_qualifier,
      const char* arg_type_name,
      cl_kernel_arg_type_qualifier arg_type_qualifier, const char* arg_name);
  void FinishRegisterKernelInfo();

  void ReleaseKernel();

  CLProgramBinary* GetBinary(CLDevice* device);
  void* GetExecutable(CLDevice* device);
  cl_build_status GetBuildStatus(CLDevice* device);
  const char* GetBuildLog(CLDevice* device);

  void* SerializeKernelInfo(CLDevice* device, size_t* size);
  void DeserializeKernelInfo(void* buffer, CLDevice* device);

 private:
  void ChangeBinary(CLDevice* device, CLProgramBinary* binary);
  void ChangeExecutable(CLDevice* device, void* executable);
  void ChangeBuildOptions(CLDevice* device, char* options);
  void ChangeBuildLog(CLDevice* device, char* log);
  void CountdownBuildCallback(CLDevice* device);

  CLContext* context_;
  std::vector<CLDevice*> devices_;

  CLProgramSource* source_;
  std::map<CLDevice*, CLProgramBinary*> binary_;
  std::map<CLDevice*, void*> executable_;
  std::vector<CLKernelInfo*> kernels_;

  int build_in_progress_cnt_;
  int build_success_cnt_;
  int kernels_ref_cnt_;

  std::map<CLDevice*, cl_build_status> build_status_;
  std::map<CLDevice*, char*> build_options_;
  std::map<CLDevice*, char*> build_log_;
  std::map<CLDevice*, ProgramCallback*> build_callback_;

  pthread_mutex_t mutex_build_;

 public:
  static CLProgram*
  CreateProgramWithSource(CLContext* context, cl_uint count,
                          const char** strings, const size_t* lengths,
                          cl_int* err);

  static CLProgram*
  CreateProgramWithBinary(CLContext* context, cl_uint num_devices,
                          const cl_device_id* device_list,
                          const size_t* lengths,
                          const unsigned char** binaries,
                          cl_int* binary_status, cl_int* err);

  static CLProgram*
  CreateProgramWithBuiltInKernels(CLContext* context, cl_uint num_devices,
                                  const cl_device_id* device_list,
                                  const char* kernel_names, cl_int* err);

  static CLProgram*
  CreateProgramWithNothing(CLContext* context, cl_uint num_devices,
                           const cl_device_id* device_list, cl_int* err);
};

#endif // __SNUCL__CL_PROGRAM_H
