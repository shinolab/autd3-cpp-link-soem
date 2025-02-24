#include <stdlib.h>

#include <iostream>

#include "autd3.hpp"
#include "autd3_link_soem.hpp"
#include "util.hpp"

int main() try {
#ifdef WIN32
  _putenv_s("RUST_LOG", "autd3=INFO");
#else
  setenv("RUST_LOG", "autd3=INFO", false);
#endif

  autd3::tracing_init();

  auto autd = autd3::Controller::open(std::vector{autd3::AUTD3()}, autd3::link::SOEM(
                                                                       [](const uint16_t slave, const autd3::link::Status status) {
                                                                         std::cout << "slave [" << slave << "]: " << status << std::endl;
                                                                         if (status == autd3::link::Status::Lost()) {
                                                                           // You can also wait for the link to recover, without exiting
                                                                           // the process
                                                                           exit(-1);
                                                                         }
                                                                       },
                                                                       autd3::link::SOEMOption{}));

  autd.send((autd3::Sine(150 * autd3::Hz, autd3::SineOption{}), autd3::Focus(autd.center() + autd3::Vector3(0.0, 0.0, 150.0), autd3::FocusOption{})));

  std::string in;
  getline(std::cin, in);

  autd.close();

  return 0;
} catch (std::exception &e) {
  print_err(e);
  return -1;
}
