/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
        * Neither the name of FastoGT. nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <common/libev/event_loop.h>

#include <stdlib.h>

#include "libev/src/ev.h"

#include <common/threads/types.h>

#include <common/libev/event_async.h>
#include <common/libev/event_io.h>
#include <common/libev/event_timer.h>

namespace common {
namespace libev {

class LibEvLoop::AsyncCustom : public LibevAsync {
 public:
  typedef unique_lock<mutex> mutex_lock_t;
  AsyncCustom() : queue_mutex_(), custom_callbacks_() {}
  ~AsyncCustom() { custom_callbacks_.clear(); }
  void Push(custom_loop_exec_function_t func) {
    {
      mutex_lock_t lock(queue_mutex_);
      custom_callbacks_.push_back(func);
    }
    Notify();
  }

  static void custom_cb(LibEvLoop* loop, LibevAsync* async, flags_t revents) {
    UNUSED(loop);
    UNUSED(revents);

    AsyncCustom* custom = static_cast<AsyncCustom*>(async);
    custom->Pop();
  }

 private:
  void Pop() {
    std::vector<custom_loop_exec_function_t> copy;
    {
      mutex_lock_t lock(queue_mutex_);
      custom_callbacks_.swap(copy);
    }

    for (size_t i = 0; i < copy.size(); ++i) {
      custom_loop_exec_function_t func = copy[i];
      func();
    }
  }
  mutex queue_mutex_;
  std::vector<custom_loop_exec_function_t> custom_callbacks_;
};

EvLoopObserver::~EvLoopObserver() {}

LibEvLoop::LibEvLoop()
    : loop_(ev_loop_new(0)),
      observer_(nullptr),
      exec_id_(),
      async_stop_(new LibevAsync),
      async_custom_(new AsyncCustom),
      timers_() {
  ev_set_userdata(loop_, this);
}

LibEvLoop::~LibEvLoop() {
  destroy(&async_custom_);
  destroy(&async_stop_);
  ev_loop_destroy(loop_);
}

void LibEvLoop::SetObserver(EvLoopObserver* observer) {
  observer_ = observer;
}

timer_id_t LibEvLoop::CreateTimer(double sec, double repeat) {
  CHECK(IsLoopThread());

  LibevTimer* timer = new LibevTimer;
  timer->Init(this, timer_cb, sec, repeat);
  timer->Start();
  timers_.push_back(timer);
  return timer->id();
}

void LibEvLoop::RemoveTimer(timer_id_t id) {
  CHECK(IsLoopThread());

  for (std::vector<LibevTimer*>::iterator it = timers_.begin(); it != timers_.end(); ++it) {
    LibevTimer* timer = *it;
    if (timer->id() == id) {
      timers_.erase(it);
      delete timer;
      return;
    }
  }
}

void LibEvLoop::InitAsync(LibevAsync* as, async_callback_t cb) {
  ev_async* eas = as->Handle();
  ev_async_init(eas, cb);
}

void LibEvLoop::StartAsync(LibevAsync* as) {
  CHECK(IsLoopThread());
  struct ev_async* eas = as->Handle();
  ev_async_start(loop_, eas);
}

void LibEvLoop::StopAsync(LibevAsync* as) {
  CHECK(IsLoopThread());
  struct ev_async* eas = as->Handle();
  ev_async_stop(loop_, eas);
}

void LibEvLoop::NotifyAsync(LibevAsync* as) {
  struct ev_async* eas = as->Handle();
  ev_async_send(loop_, eas);
}

void LibEvLoop::InitIO(LibevIO* io, io_callback_t cb, descriptor_t fd, flags_t events) {
  ev_io* eio = io->Handle();
  ev_io_init(eio, cb, fd, events);
}

void LibEvLoop::StartIO(LibevIO* io) {
  CHECK(IsLoopThread());
  ev_io* eio = io->Handle();
  ev_io_start(loop_, eio);
}

void LibEvLoop::StopIO(LibevIO* io) {
  CHECK(IsLoopThread());
  ev_io* eio = io->Handle();
  ev_io_stop(loop_, eio);
}

void LibEvLoop::InitTimer(LibevTimer* timer, timer_callback_t cb, double sec, double repeat) {
  ev_timer* eit = timer->Handle();
  ev_timer_init(eit, cb, sec, repeat);
}

void LibEvLoop::StartTimer(LibevTimer* timer) {
  CHECK(IsLoopThread());
  ev_timer* eit = timer->Handle();
  ev_timer_start(loop_, eit);
}

void LibEvLoop::StopTimer(LibevTimer* timer) {
  CHECK(IsLoopThread());
  ev_timer* eit = timer->Handle();
  ev_timer_stop(loop_, eit);
}

void LibEvLoop::ExecInLoopThread(custom_loop_exec_function_t func) {
  if (IsLoopThread()) {
    func();
    return;
  }

  async_custom_->Push(func);
}

int LibEvLoop::Exec() {
  exec_id_ = threads::PlatformThread::GetCurrentId();

  async_custom_->Init(this, AsyncCustom::custom_cb);
  async_custom_->Start();
  async_stop_->Init(this, stop_cb);
  async_stop_->Start();
  if (observer_) {
    observer_->PreLooped(this);
  }
  ev_loop(loop_, 0);
  if (observer_) {
    observer_->PostLooped(this);
  }
  return EXIT_SUCCESS;
}

bool LibEvLoop::IsLoopThread() const {
  return exec_id_ == threads::PlatformThread::GetCurrentId();
}

void LibEvLoop::Stop() {
  async_stop_->Notify();
}

void LibEvLoop::stop_cb(LibEvLoop* loop, LibevAsync* async, flags_t revents) {
  UNUSED(async);
  UNUSED(revents);

  loop->HandleStop();
}

void LibEvLoop::timer_cb(LibEvLoop* loop, LibevTimer* timer, flags_t revents) {
  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  DCHECK(revents & EV_TIMEOUT);
  loop->HandleTimer(timer->id());
}

void LibEvLoop::HandleTimer(timer_id_t id) {
  if (observer_) {
    observer_->TimerEmited(this, id);
  }
}

void LibEvLoop::HandleStop() {
  async_stop_->Stop();
  const std::vector<LibevTimer*> timers = timers_;
  for (size_t i = 0; i < timers.size(); ++i) {
    LibevTimer* timer = timers[i];
    RemoveTimer(timer->id());
  }
  if (observer_) {
    observer_->Stoped(this);
  }
  ev_unloop(loop_, EVUNLOOP_ONE);
}

}  // namespace libev
}  // namespace common
