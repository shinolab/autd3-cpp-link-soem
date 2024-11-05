#pragma once

#include <exception>
#include <iostream>

#include "autd3.hpp"

#ifdef WIN32
FORCE_CODEPAGE_UTF8_WIN
#endif

void print_err(std::exception& e) {
  std::cerr << "\033[91m"
            << "ERROR: " << e.what() << "\033[0m" << std::endl;
}
