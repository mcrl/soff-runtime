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

#include "CLIssuer.h"
#include <algorithm>
#include <list>
#include <vector>
#include <pthread.h>
#include "CLCommand.h"
#include "CLDevice.h"
#include "CLEvent.h"

using namespace std;

CLIssuer::CLIssuer(CLDevice* device, bool blocking) {
  blocking_ = blocking;
  devices_.push_back(device);
  devices_updated_ = true;

  thread_ = (pthread_t)NULL;
  thread_running_ = false;

  pthread_mutex_init(&mutex_devices_, NULL);
  pthread_cond_init(&cond_devices_remove_, NULL);
}

CLIssuer::CLIssuer(bool blocking) {
  blocking_ = blocking;

  thread_ = (pthread_t)NULL;
  thread_running_ = false;

  pthread_mutex_init(&mutex_devices_, NULL);
  pthread_cond_init(&cond_devices_remove_, NULL);
}

CLIssuer::~CLIssuer() {
  Stop();

  pthread_mutex_destroy(&mutex_devices_);
  pthread_cond_destroy(&cond_devices_remove_);
}

void CLIssuer::Start() {
  if (!thread_) {
    thread_running_ = true;
    pthread_create(&thread_, NULL, &CLIssuer::ThreadFunc, this);
  }
}

void CLIssuer::Stop() {
  if (thread_) {
    thread_running_ = false;
    pthread_mutex_lock(&mutex_devices_);
    for (vector<CLDevice*>::iterator it = devices_.begin();
         it != devices_.end();
         ++it) {
      (*it)->InvokeReadyQueue();
    }
    pthread_mutex_unlock(&mutex_devices_);
    pthread_join(thread_, NULL);
    thread_ = (pthread_t)NULL;
  }
}

CLDevice* CLIssuer::GetFirstDevice() {
  pthread_mutex_lock(&mutex_devices_);
  CLDevice* device = devices_.front();
  pthread_mutex_unlock(&mutex_devices_);
  return device;
}

void CLIssuer::AddDevice(CLDevice* device) {
  if (blocking_) return;

  pthread_mutex_lock(&mutex_devices_);
  devices_updated_ = true;
  devices_.push_back(device);
  pthread_mutex_unlock(&mutex_devices_);
}

void CLIssuer::RemoveDevice(CLDevice* device) {
  if (blocking_) return;

  pthread_mutex_lock(&mutex_devices_);
  vector<CLDevice*>::iterator it = find(devices_.begin(), devices_.end(),
                                        device);
  if (it != devices_.end()) {
    devices_updated_ = true;
    *it = NULL;
    for (vector<CLDevice*>::iterator other_it = devices_.begin();
         other_it != devices_.end();
         ++other_it) {
      if (*other_it != NULL)
        (*other_it)->InvokeReadyQueue();
    }
    pthread_cond_wait(&cond_devices_remove_, &mutex_devices_);
  }
  pthread_mutex_unlock(&mutex_devices_);
}

void CLIssuer::Run() {
  vector<CLDevice*> target_devices;
  while (thread_running_) {
    if (devices_updated_) {
      pthread_mutex_lock(&mutex_devices_);
      pthread_cond_broadcast(&cond_devices_remove_);
      devices_.erase(remove(devices_.begin(), devices_.end(), (CLDevice*)NULL),
                     devices_.end());
      target_devices = devices_;
      devices_updated_ = false;
      pthread_mutex_unlock(&mutex_devices_);
    }

    for (vector<CLDevice*>::iterator it = target_devices.begin();
         it != target_devices.end();
         ++it) {
      CLDevice* device = *it;
      if (blocking_)
        device->WaitReadyQueue();
      CLCommand* command = device->DequeueReadyQueue();
      if (command != NULL) {
        command->SetAsRunning();
        if (!blocking_) {
          running_commands_.push_back(command);
          command->Execute();
        } else {
          command->Execute();
          command->SetAsComplete();
          delete command;
        }
      }
    }

    if (!blocking_) {
      list<CLCommand*>::iterator it = running_commands_.begin();
      while (it != running_commands_.end()) {
        CLCommand* command = *it;
        if (command->device()->IsComplete(command)) {
          command->SetAsComplete();
          it = running_commands_.erase(it);
          delete command;
        } else {
          ++it;
        }
      }
    }
  }
}

void* CLIssuer::ThreadFunc(void *argp) {
  ((CLIssuer*)argp)->Run();
  return NULL;
}
