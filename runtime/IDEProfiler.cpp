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

#include "IDEProfiler.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <CL/cl.h>
#include "CLCommandQueue.h"
#include "CLEvent.h"

using namespace std;

static void ide_profiler_exit() {
  delete IDEProfiler::GetProfiler();
}

IDEProfiler::IDEProfiler() {
  char* enable = getenv("SNUCL_IDE_ENABLE_PROFILING");
  enabled_ = (enable != NULL && strcmp(enable, "1") == 0);
  if (enabled_) {
    atexit(ide_profiler_exit);
  }
}

IDEProfiler::~IDEProfiler() {
  if (enabled_) {
    RecordEventProfilingOutput();
  }
}

void IDEProfiler::Register(CLEvent* event) {
  if (enabled_) {
    IDEProfilerEventEntry entry;
    if (event->queue() == NULL)
      return;
    entry.queue_id = event->queue()->id();
    entry.event_id = event->id();
    event->GetEventInfo(CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type),
                        &entry.type, NULL);
    event->GetEventProfilingInfo(CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong),
                                 &entry.profile[0], NULL);
    event->GetEventProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong),
                                 &entry.profile[1], NULL);
    event->GetEventProfilingInfo(CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
                                 &entry.profile[2], NULL);
    event->GetEventProfilingInfo(CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
                                 &entry.profile[3], NULL);
    events_.push_back(entry);
  }
}

static bool entry_compare(IDEProfilerEventEntry a, IDEProfilerEventEntry b) {
  if (a.queue_id != b.queue_id)
    return a.queue_id < b.queue_id;
  else
    return a.event_id < b.event_id;
}

void IDEProfiler::RecordEventProfilingOutput() {
  sort(events_.begin(), events_.end(), entry_compare);
  FILE* fp = fopen("result.epo", "wb");
  if (fp == NULL)
    return;
  int version = 0;
  fwrite(&version, sizeof(int), 1, fp);
  for (size_t i = 0; i < events_.size(); i++) {
    IDEProfilerEventEntry entry = events_[i];
    entry.event_id = i + 1; // assign a new (1-based) id
    fwrite(&entry, sizeof(entry), 1, fp);
  }
  fclose(fp);
}

IDEProfiler* IDEProfiler::singleton_ = NULL;

IDEProfiler* IDEProfiler::GetProfiler() {
  if (singleton_ == NULL) {
    singleton_ = new IDEProfiler();
  }
  return singleton_;
}
