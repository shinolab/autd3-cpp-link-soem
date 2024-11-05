#include <stdlib.h>

#include <iostream>

#include "autd3-link-soem.hpp"
#include "autd3.hpp"
#include "util.hpp"

int main() try {
#ifdef WIN32
  _putenv_s("RUST_LOG", "autd3=INFO");
#else
  setenv("RUST_LOG", "autd3=INFO", false);
#endif

  autd3::tracing_init();

  auto autd = autd3::ControllerBuilder(std::vector{autd3::AUTD3(autd3::Vector3::Zero())})
                  .open(autd3::link::SOEM::builder().with_err_handler([](const uint16_t slave, const autd3::link::Status status) {
                    std::cout << "slave [" << slave << "]: " << status << std::endl;
                    if (status == autd3::link::Status::Lost()) {
                      // You can also wait for the link to recover, without exiting the process
                      exit(-1);
                    }
                  }));

  autd.send((autd3::modulation::Sine(150 * autd3::Hz), autd3::gain::Focus(autd.center() + autd3::Vector3(0.0, 0.0, 150.0))));

  std::string in;
  getline(std::cin, in);

  autd.close();

  return 0;
} catch (std::exception& e) {
  print_err(e);
  return -1;
}
