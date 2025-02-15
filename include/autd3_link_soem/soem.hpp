#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "autd3/native_methods/utils.hpp"
#include "autd3_link_soem/native_methods.hpp"

namespace autd3::link {

using native_methods::ProcessPriority;
using native_methods::SyncMode;
using native_methods::TimerStrategy;

class Status {
  native_methods::Status _inner;
  std::string _msg;

  explicit Status(const native_methods::Status inner, std::string msg) : _inner(inner), _msg(std::move(msg)) {}

 public:
  friend struct SOEM;

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

struct ThreadPriority {
  AUTD3_API static ThreadPriority Min() { return ThreadPriority(native_methods::AUTDLinkSOEMThreadPriorityMin()); }
  AUTD3_API static ThreadPriority Max() { return ThreadPriority(native_methods::AUTDLinkSOEMThreadPriorityMax()); }
  AUTD3_API [[nodiscard]] static ThreadPriority Crossplarform(const uint8_t value) {
    if (value > 99) throw std::invalid_argument("value must be between 0 and 99");
    return ThreadPriority(native_methods::AUTDLinkSOEMThreadPriorityCrossplatform(value));
  }

  operator native_methods::ThreadPriorityPtr() const { return _ptr; }

 private:
  explicit ThreadPriority(const native_methods::ThreadPriorityPtr ptr) : _ptr(ptr) {}

  native_methods::ThreadPriorityPtr _ptr;
};

template <class F>
concept soem_err_handler_f = requires(F f, const uint16_t slave, const Status status) {
  { f(slave, status) } -> std::same_as<void>;
};

struct SOEMOption {
  size_t buf_size = 32;
  TimerStrategy timer_strategy = TimerStrategy::SpinSleep;
  SyncMode sync_mode = SyncMode::DC;
  std::string ifname = "";
  std::chrono::nanoseconds state_check_interval = std::chrono::milliseconds(100);
  std::chrono::nanoseconds sync0_cycle = std::chrono::milliseconds(1);
  std::chrono::nanoseconds send_cycle = std::chrono::milliseconds(1);
  ThreadPriority thread_priority = ThreadPriority::Max();
  ProcessPriority process_priority = ProcessPriority::High;
  std::chrono::nanoseconds sync_tolerance = std::chrono::microseconds(1);
  std::chrono::nanoseconds sync_timeout = std::chrono::seconds(10);

  operator native_methods::SOEMOption() const {
    return native_methods::SOEMOption{.ifname = ifname.c_str(),
                                      .buf_size = static_cast<uint32_t>(buf_size),
                                      .send_cycle = native_methods::to_duration(send_cycle),
                                      .sync0_cycle = native_methods::to_duration(sync0_cycle),
                                      .sync_mode = SyncMode::DC,
                                      .process_priority = process_priority,
                                      .thread_priority = thread_priority,
                                      .state_check_interval = native_methods::to_duration(state_check_interval),
                                      .timer_strategy = timer_strategy,
                                      .sync_tolerance = native_methods::to_duration(sync_tolerance),
                                      .sync_timeout = native_methods::to_duration(sync_timeout)};
  }
};

struct SOEM final {
  using err_handler_t = void (*)(uint16_t, Status);

  explicit SOEM(const err_handler_t err_handler, const SOEMOption option) : err_handler(err_handler), option(option) {
    _native_err_handler = +[](const void* context, const uint32_t slave, const native_methods::Status status) {           // LCOV_EXCL_LINE
      const std::string msg(128, ' ');                                                                                    // LCOV_EXCL_LINE
      (void)AUTDLinkSOEMStatusGetMsg(status, const_cast<char*>(msg.c_str()));                                             // LCOV_EXCL_LINE
      (*reinterpret_cast<err_handler_t>(const_cast<void*>(context)))(static_cast<uint16_t>(slave), Status(status, msg));  // LCOV_EXCL_LINE
    };  // LCOV_EXCL_LINE
  }

  [[nodiscard]] native_methods::LinkPtr resolve() const {
    return validate(native_methods::AUTDLinkSOEM(reinterpret_cast<void*>(_native_err_handler), reinterpret_cast<void*>(err_handler), option));
  }

  err_handler_t err_handler;
  SOEMOption option;

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

 private:
  using native_err_handler_t = void (*)(const void*, uint32_t, native_methods::Status);
  native_err_handler_t _native_err_handler;
};

struct RemoteSOEM final {
  explicit RemoteSOEM(std::string addr) : addr(std::move(addr)) {}

  std::string addr;

  [[nodiscard]] native_methods::LinkPtr resolve() { return validate(native_methods::AUTDLinkRemoteSOEM(addr.c_str())); }
};

}  // namespace autd3::link
