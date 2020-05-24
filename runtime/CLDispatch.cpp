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

#include "CLDispatch.h"
#include <stdlib.h>
#include "CLAPI.h"
#include "ICD.h"

CLDispatch::CLDispatch() {
  dispatch_ =
    (struct _cl_icd_dispatch*)malloc(sizeof(struct _cl_icd_dispatch));

#define SET_DISPATCH(func) dispatch_->func = SNUCL_API_FUNCTION(func)

  SET_DISPATCH(clGetPlatformIDs);
  SET_DISPATCH(clGetPlatformInfo);
  SET_DISPATCH(clGetDeviceIDs);
  SET_DISPATCH(clGetDeviceInfo);
  SET_DISPATCH(clCreateContext);
  SET_DISPATCH(clCreateContextFromType);
  SET_DISPATCH(clRetainContext);
  SET_DISPATCH(clReleaseContext);
  SET_DISPATCH(clGetContextInfo);
  SET_DISPATCH(clCreateCommandQueue);
  SET_DISPATCH(clRetainCommandQueue);
  SET_DISPATCH(clReleaseCommandQueue);
  SET_DISPATCH(clGetCommandQueueInfo);
  SET_DISPATCH(clSetCommandQueueProperty);
  SET_DISPATCH(clCreateBuffer);
  SET_DISPATCH(clCreateImage2D);
  SET_DISPATCH(clCreateImage3D);
  SET_DISPATCH(clRetainMemObject);
  SET_DISPATCH(clReleaseMemObject);
  SET_DISPATCH(clGetSupportedImageFormats);
  SET_DISPATCH(clGetMemObjectInfo);
  SET_DISPATCH(clGetImageInfo);
  SET_DISPATCH(clCreateSampler);
  SET_DISPATCH(clRetainSampler);
  SET_DISPATCH(clReleaseSampler);
  SET_DISPATCH(clGetSamplerInfo);
  SET_DISPATCH(clCreateProgramWithSource);
  SET_DISPATCH(clCreateProgramWithBinary);
  SET_DISPATCH(clRetainProgram);
  SET_DISPATCH(clReleaseProgram);
  SET_DISPATCH(clBuildProgram);
  SET_DISPATCH(clUnloadCompiler);
  SET_DISPATCH(clGetProgramInfo);
  SET_DISPATCH(clGetProgramBuildInfo);
  SET_DISPATCH(clCreateKernel);
  SET_DISPATCH(clCreateKernelsInProgram);
  SET_DISPATCH(clRetainKernel);
  SET_DISPATCH(clReleaseKernel);
  SET_DISPATCH(clSetKernelArg);
  SET_DISPATCH(clGetKernelInfo);
  SET_DISPATCH(clGetKernelWorkGroupInfo);
  SET_DISPATCH(clWaitForEvents);
  SET_DISPATCH(clGetEventInfo);
  SET_DISPATCH(clRetainEvent);
  SET_DISPATCH(clReleaseEvent);
  SET_DISPATCH(clGetEventProfilingInfo);
  SET_DISPATCH(clFlush);
  SET_DISPATCH(clFinish);
  SET_DISPATCH(clEnqueueReadBuffer);
  SET_DISPATCH(clEnqueueWriteBuffer);
  SET_DISPATCH(clEnqueueCopyBuffer);
  SET_DISPATCH(clEnqueueReadImage);
  SET_DISPATCH(clEnqueueWriteImage);
  SET_DISPATCH(clEnqueueCopyImage);
  SET_DISPATCH(clEnqueueCopyImageToBuffer);
  SET_DISPATCH(clEnqueueCopyBufferToImage);
  SET_DISPATCH(clEnqueueMapBuffer);
  SET_DISPATCH(clEnqueueMapImage);
  SET_DISPATCH(clEnqueueUnmapMemObject);
  SET_DISPATCH(clEnqueueNDRangeKernel);
  SET_DISPATCH(clEnqueueTask);
  SET_DISPATCH(clEnqueueNativeKernel);
  SET_DISPATCH(clEnqueueMarker);
  SET_DISPATCH(clEnqueueWaitForEvents);
  SET_DISPATCH(clEnqueueBarrier);
  SET_DISPATCH(clGetExtensionFunctionAddress);

#if 0
  SET_DISPATCH(clCreateFromGLBuffer);
  SET_DISPATCH(clCreateFromGLTexture);
  SET_DISPATCH(clCreateFromGLRenderbuffer);
  SET_DISPATCH(clGetGLObjectInfo);
  SET_DISPATCH(clGetGLTextureInfo);
  SET_DISPATCH(clEnqueueAcquireGLObjects);
  SET_DISPATCH(clEnqueueReleaseGLObjects);
  SET_DISPATCH(clGetGLContextInfoKRH);
  SET_DISPATCH(clCreateEventFromGLsyncKHR);
  SET_DISPATCH(clCreateFromGLTexture2D);
  SET_DISPATCH(clCreateFromGLTexture3D);

  SET_DISPATCH(clGetDeviceIDsFromD3D10);
  SET_DISPATCH(clCreateFromD3D10Buffer);
  SET_DISPATCH(clCreateFromD3D10Texture2D);
  SET_DISPATCH(clCreateFromD3D10Texture3D);
  SET_DISPATCH(clEnqueueAcquireD3D10Objects);
  SET_DISPATCH(clEnqueueReleaseD3D10Objects);
#endif

  SET_DISPATCH(clSetEventCallback);
  SET_DISPATCH(clCreateSubBuffer);
  SET_DISPATCH(clSetMemObjectDestructorCallback);
  SET_DISPATCH(clCreateUserEvent);
  SET_DISPATCH(clSetUserEventStatus);
  SET_DISPATCH(clEnqueueReadBufferRect);
  SET_DISPATCH(clEnqueueWriteBufferRect);
  SET_DISPATCH(clEnqueueCopyBufferRect);

#if 0
  SET_DISPATCH(clCreateSubDevicesEXT);
  SET_DISPATCH(clRetainDeviceEXT);
  SET_DISPATCH(clReleaseDeviceEXT);

  SET_DISPATCH(clCreateEventFromGLsyncKHR);
#endif

  SET_DISPATCH(clCreateSubDevices);
  SET_DISPATCH(clRetainDevice);
  SET_DISPATCH(clReleaseDevice);
  SET_DISPATCH(clCreateImage);
  SET_DISPATCH(clCreateProgramWithBuiltInKernels);
  SET_DISPATCH(clCompileProgram);
  SET_DISPATCH(clLinkProgram);
  SET_DISPATCH(clUnloadPlatformCompiler);
  SET_DISPATCH(clGetKernelArgInfo);
  SET_DISPATCH(clEnqueueFillBuffer);
  SET_DISPATCH(clEnqueueFillImage);
  SET_DISPATCH(clEnqueueMigrateMemObjects);
  SET_DISPATCH(clEnqueueMarkerWithWaitList);
  SET_DISPATCH(clEnqueueBarrierWithWaitList);
  SET_DISPATCH(clGetExtensionFunctionAddressForPlatform);

#if 0
  SET_DISPATCH(clCreateFromGLTexture);

  SET_DISPATCH(clGetDeviceIDsFromD3D11KHR);
  SET_DISPATCH(clCreateFromD3D11BufferKHR);
  SET_DISPATCH(clCreateFromD3D11Texture2DKHR);
  SET_DISPATCH(clCreateFromD3D11Texture3DKHR);
  SET_DISPATCH(clCreateFromDX9MediaSurfaceKHR);
  SET_DISPATCH(clEnqueueAcquireD3D11ObjectsKHR);
  SET_DISPATCH(clEnqueueReleaseD3D11ObjectsKHR);

  SET_DISPATCH(clGetDeviceIDsFromDX9MediaAdapterKHR);
  SET_DISPATCH(clEnqueueAcquireDX9MediaSurfacesKHR);
  SET_DISPATCH(clEnqueueReleaseDX9MediaSurfacesKHR);
#endif

#undef SET_DISPATCH
}

CLDispatch::~CLDispatch() {
  free(dispatch_);
}

CLDispatch* CLDispatch::singleton_ = NULL;

struct _cl_icd_dispatch* CLDispatch::GetDispatch() {
  if (singleton_ == NULL)
    singleton_ = new CLDispatch();
  return singleton_->dispatch_;
}
