#include <gtest/gtest.h>

#include <autd3_link_soem/soem.hpp>
#include <sstream>

TEST(Link, ThreadPriority) {
  (void)autd3::link::ThreadPriority::Max;
  (void)autd3::link::ThreadPriority::Min;
  (void)autd3::link::ThreadPriority::Crossplarform(0);
  (void)autd3::link::ThreadPriority::Crossplarform(99);
  ASSERT_THROW((void)autd3::link::ThreadPriority::Crossplarform(100), std::invalid_argument);
}

TEST(Link, Status) {
  const auto lost = autd3::link::Status::Lost();
  const auto state_change = autd3::link::Status::StateChanged();
  const auto err = autd3::link::Status::Error();

  ASSERT_EQ(lost, autd3::link::Status::Lost());
  ASSERT_EQ(state_change, autd3::link::Status::StateChanged());
  ASSERT_EQ(err, autd3::link::Status::Error());
  ASSERT_NE(lost, state_change);
  ASSERT_NE(lost, err);
  ASSERT_NE(state_change, err);
  ASSERT_NE(state_change, lost);
  ASSERT_NE(err, lost);
  ASSERT_NE(err, state_change);

  std::stringstream ss;
  ss << lost;
  ASSERT_EQ(ss.str(), "");
}

TEST(Link, SOEMIsDefault) { ASSERT_TRUE(autd3::native_methods::AUTDLinkSOEMIsDefault(autd3::link::SOEMOption{})); }

TEST(Link, SOEM) {
  auto link = autd3::link::SOEM([](const uint16_t, const autd3::link::Status) {}, autd3::link::SOEMOption{});
  (void)link;
}

TEST(Link, RemoteSOEM) {
  auto link = autd3::link::RemoteSOEM("127.0.0.1:8080");
  (void)link;
}
