#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "autd3/native_methods/utils.hpp"
#include "autd3_link_soem/native_methods.hpp"

namespace autd3::controller {
class ControllerBuilder;
}

namespace autd3::link {

using native_methods::ProcessPriority;
using native_methods::SyncMode;
using native_methods::TimerStrategy;

class Status {
  native_methods::Status _inner;
  std::string _msg;

  explicit Status(const native_methods::Status inner, std::string msg) : _inner(inner), _msg(std::move(msg)) {}

 public:
  friend class SOEM;

  static Status Lost() { return Status(native_methods::Status::Lost, ""); }
  static Status Error() { return Status(native_methods::Status::Error, ""); }
  static Status StateChanged() { return Status(native_methods::Status::StateChanged, ""); }

  bool operator==(const Status& that) const { return _inner == that._inner && _msg == that._msg; }

  friend std::ostream& operator<<(std::ostream& os, const Status& s);
};

inline std::ostream& operator<<(std::ostream& os, const Status& s) {
  os << s._msg;
  return os;
}

class EtherCATAdapter {
  std::string _desc;
  std::string _name;

 public:
  EtherCATAdapter(std::string desc, std::string name) : _desc(std::move(desc)), _name(std::move(name)) {}

  [[nodiscard]] const std::string& desc() const { return _desc; }
  [[nodiscard]] const std::string& name() const { return _name; }
};

class ThreadPriority {
 public:
  AUTD3_API static inline const native_methods::ThreadPriorityPtr Min = native_methods::AUTDLinkSOEMThreadPriorityMin();
  AUTD3_API static inline const native_methods::ThreadPriorityPtr Max = native_methods::AUTDLinkSOEMThreadPriorityMax();
  AUTD3_API [[nodiscard]] static native_methods::ThreadPriorityPtr Crossplarform(const uint8_t value) {
    if (value > 99) throw std::invalid_argument("value must be between 0 and 99");
    return native_methods::AUTDLinkSOEMThreadPriorityCrossplatform(value);
  }
};

template <class F>
concept soem_err_handler_f = requires(F f, const uint16_t slave, const Status status) {
  { f(slave, status) } -> std::same_as<void>;
};

class SOEM final {
  using native_err_handler_t = void (*)(const void*, uint32_t, native_methods::Status);
  using err_handler_t = void (*)(uint16_t, Status);

  explicit SOEM(const native_err_handler_t native_err_handler, const err_handler_t err_handler)
      : _native_err_handler(native_err_handler), _err_handler(err_handler) {}

  [[maybe_unused]] native_err_handler_t _native_err_handler;
  [[maybe_unused]] err_handler_t _err_handler;

 public:
  class Builder final {
    friend class SOEM;
    friend class controller::ControllerBuilder;

    native_err_handler_t _native_err_handler = nullptr;

    AUTD3_API Builder()
        : _buf_size(32),
          _send_cycle(std::chrono::milliseconds(1)),
          _sync0_cycle(std::chrono::milliseconds(1)),
          _timer_strategy(TimerStrategy::SpinSleep),
          _sync_mode(SyncMode::DC),
          _sync_tolerance(std::chrono::microseconds(1)),
          _sync_timeout(std::chrono::seconds(10)),
          _state_check_interval(std::chrono::milliseconds(100)),
          _thread_priority(ThreadPriority::Max),
          _process_priority(ProcessPriority::High),
          _err_handler(nullptr) {}

    [[nodiscard]] SOEM resolve_link(native_methods::HandlePtr, native_methods::LinkPtr) const { return SOEM{_native_err_handler, _err_handler}; }

   public:
    using Link = SOEM;

    AUTD3_DEF_PARAM(Builder, std::string, ifname)
    AUTD3_DEF_PARAM(Builder, size_t, buf_size)
    AUTD3_DEF_PARAM_CHRONO(Builder, send_cycle)
    AUTD3_DEF_PARAM_CHRONO(Builder, sync0_cycle)
    AUTD3_DEF_PARAM(Builder, TimerStrategy, timer_strategy)
    AUTD3_DEF_PARAM(Builder, SyncMode, sync_mode)
    AUTD3_DEF_PARAM_CHRONO(Builder, sync_tolerance)
    AUTD3_DEF_PARAM_CHRONO(Builder, sync_timeout)
    AUTD3_DEF_PARAM_CHRONO(Builder, state_check_interval)
    AUTD3_DEF_PARAM(Builder, native_methods::ThreadPriorityPtr, thread_priority)
    AUTD3_DEF_PARAM(Builder, ProcessPriority, process_priority)

    err_handler_t _err_handler;

    [[nodiscard]] native_methods::LinkBuilderPtr ptr() const {
      return native_methods::LinkBuilderPtr{
          validate(AUTDLinkSOEM(_ifname.c_str(), static_cast<uint32_t>(_buf_size),
                                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_send_cycle).count()),
                                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_sync0_cycle).count()),
                                reinterpret_cast<void*>(_native_err_handler), reinterpret_cast<void*>(_err_handler), _sync_mode, _process_priority,
                                _thread_priority,
                                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_state_check_interval).count()),
                                _timer_strategy, static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_sync_tolerance).count()),
                                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_sync_timeout).count())))
              ._0};
    }

    template <soem_err_handler_f F>
    AUTD3_API [[nodiscard]] Builder&& with_err_handler(F value) && {
      _err_handler = static_cast<err_handler_t>(value);
      _native_err_handler = +[](const void* context, const uint32_t slave, const native_methods::Status status) {
        const std::string msg(128, ' ');                                                                                    // LCOV_EXCL_LINE
        (void)AUTDLinkSOEMStatusGetMsg(status, const_cast<char*>(msg.c_str()));                                             // LCOV_EXCL_LINE
        (*reinterpret_cast<err_handler_t>(const_cast<void*>(context)))(static_cast<uint16_t>(slave), Status(status, msg));  // LCOV_EXCL_LINE
      };  // LCOV_EXCL_LINE
      return std::move(*this);
    }
  };

  AUTD3_API [[nodiscard]] static Builder builder() { return {}; }

  [[nodiscard]] static std::vector<EtherCATAdapter> enumerate_adapters() {
    const auto handle = native_methods::AUTDAdapterPointer();
    const auto len = AUTDAdapterGetSize(handle);
    std::vector<EtherCATAdapter> adapters;
    for (uint32_t i = 0; i < len; i++) {
      char sb_desc[128];
      char sb_name[128];
      AUTDAdapterGetAdapter(handle, i, sb_desc, sb_name);
      adapters.emplace_back(std::string(sb_desc), std::string(sb_name));
    }
    AUTDAdapterPointerDelete(handle);
    return adapters;
  }
};

class RemoteSOEM final {
  RemoteSOEM() = default;  // LCOV_EXCL_LINE

 public:
  class Builder final {
    friend class RemoteSOEM;
    friend class controller::ControllerBuilder;

    AUTD3_API explicit Builder(std::string addr) : _addr(std::move(addr)) {}

    [[nodiscard]] static RemoteSOEM resolve_link(native_methods::HandlePtr, native_methods::LinkPtr) { return RemoteSOEM{}; }

   public:
    using Link = RemoteSOEM;

    AUTD3_DEF_PROP(std::string, addr)

    [[nodiscard]] native_methods::LinkBuilderPtr ptr() const {
      return native_methods::LinkBuilderPtr{validate(native_methods::AUTDLinkRemoteSOEM(_addr.c_str()))._0};
    }
  };

  AUTD3_API [[nodiscard]] static Builder builder(const std::string& addr) { return Builder(addr); }
};

}  // namespace autd3::link
