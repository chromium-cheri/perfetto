/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_PERFETTO_TRACING_TRACK_EVENT_ARGS_H_
#define INCLUDE_PERFETTO_TRACING_TRACK_EVENT_ARGS_H_

#include "perfetto/tracing/event_context.h"
#include "perfetto/tracing/track.h"

#include <functional>

namespace perfetto {

// A helper to add |flow_id| as a non-terminating flow id to TRACE_EVENT
// inline: TRACE_EVENT(..., perfetto::Flow::ProcessScoped(42));
class Flow {
 public:
  // |flow_id| which is local within a given process (e.g. atomic counter xor'ed
  // with feature-specific value). This value is xor'ed with Perfetto's internal
  // process track id to attempt to ensure that it's globally-unique.
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  ProcessScoped(uint64_t flow_id) {
    return Global(flow_id ^ Track::process_uuid);
  }

  // Same as above, but construct an id from a pointer.
  // NOTE: After the object is destroyed, the value of |ptr| can be reused for a
  // different object (in particular if the object is allocated on a stack).
  // Please ensure that you emit a trace event with the flow id of
  // perfetto::TerminatingFlow::FromPointer(this) from the destructor of the
  // object to avoid accidental conflicts.
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  FromPointer(void* ptr) {
    return ProcessScoped(reinterpret_cast<uintptr_t>(ptr));
  }

  // Add the |flow_id|. The caller is responsible for ensuring that it's
  // globally-unique (e.g. by generating a random value). This should be used
  // only for flow events which cross the process boundary (e.g. IPCs).
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  Global(uint64_t flow_id) {
    return [flow_id](perfetto::EventContext& ctx) {
      ctx.event()->add_flow_ids(flow_id);
    };
  }

  // TODO(altimin): Remove once converting a single usage in Chromium.
  explicit constexpr Flow(uint64_t flow_id) : flow_id_(flow_id) {}

  void operator()(EventContext& ctx) { ctx.event()->add_flow_ids(flow_id_); }

 private:
  uint64_t flow_id_;
};

// A helper to add a given |flow_id| as a terminating flow to TRACE_EVENT
// inline.
class TerminatingFlow {
 public:
  // See `Flow::ProcessScoped(uint64_t)`.
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  ProcessScoped(uint64_t flow_id) {
    return Global(flow_id ^ Track::process_uuid);
  }

  // See `Flow::FromPointer(void*)`.
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  FromPointer(void* ptr) {
#if defined(__CHERI_PURE_CAPABILITY__)
    return ProcessScoped(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr)));
#else   // !__CHERI_PURE_CAPABILITY__
    return ProcessScoped(reinterpret_cast<uintptr_t>(ptr));
#endif  // !__CHERI_PURE_CAPABILITY__
  }

  // See `Flow::Global(uint64_t)`.
  static PERFETTO_ALWAYS_INLINE inline std::function<void(EventContext&)>
  Global(uint64_t flow_id) {
    return [flow_id](perfetto::EventContext& ctx) {
      ctx.event()->add_terminating_flow_ids(flow_id);
    };
  }
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_TRACK_EVENT_ARGS_H_
