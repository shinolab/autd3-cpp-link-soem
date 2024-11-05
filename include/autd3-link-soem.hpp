#pragma once

#include "autd3-link-soem/soem.hpp"

namespace autd3 {

namespace link::soem {
static inline std::string version = "29.0.0-rc.5";
}

inline void tracing_init() {
  native_methods::AUTDTracingInit();
  native_methods::AUTDLinkSOEMTracingInit();
}

}  // namespace autd3
