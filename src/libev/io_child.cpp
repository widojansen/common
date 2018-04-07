/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <common/libev/io_child.h>

#if LIBEV_CHILD_ENABLE

#include <inttypes.h>

#include <common/libev/event_child.h>
#include <common/libev/io_loop.h>
#include <common/sprintf.h>

namespace common {
namespace libev {

IoChild::IoChild(IoLoop* server) : server_(server), child_(new LibevChild), name_(), id_() {
  child_->SetUserData(this);
}

IoChild::~IoChild() {
  destroy(&child_);
}

patterns::id_counter<IoChild>::type_t IoChild::GetId() const {
  return id_.get_id();
}

pid_t IoChild::GetPid() const {
  return child_->GetPid();
}

IoLoop* IoChild::GetServer() const {
  return server_;
}

void IoChild::SetName(const std::string& name) {
  name_ = name;
}

std::string IoChild::GetName() const {
  return name_;
}

const char* IoChild::ClassName() const {
  return "IoClient";
}

std::string IoChild::GetFormatedName() const {
  return MemSPrintf("[%s][%s(%" PRIuMAX ")]", GetName(), ClassName(), GetId());
}

}  // namespace libev
}  // namespace common

#endif
