#include "autd3-link-soem.hpp"
#include "autd3.hpp"
#include "util.hpp"

int main() try {
  auto autd = autd3::ControllerBuilder(std::vector{autd3::AUTD3(autd3::Vector3::Zero())}).open(autd3::link::RemoteSOEM::builder("127.0.0.1:8080"));

  autd.send((autd3::modulation::Sine(150 * autd3::Hz), autd3::gain::Focus(autd.center() + autd3::Vector3(0.0, 0.0, 150.0))));

  std::string in;
  getline(std::cin, in);

  autd.close();

  return 0;
} catch (std::exception& e) {
  print_err(e);
  return -1;
}
