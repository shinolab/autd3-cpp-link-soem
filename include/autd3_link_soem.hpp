#pragma once

#include "autd3_link_soem/soem.hpp"

namespace autd3 {

namespace link::soem {
static inline std::string version = "35.0.1";
}

inline void tracing_init() { native_methods::AUTDLinkSOEMTracingInit(); }

}  // namespace autd3
