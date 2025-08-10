#include "../src/statistic_app.hpp"
#include <algorithm>
#include <cassert>

void test_valid_ip_port() {
  sockaddr_in addr{};
  auto err = convert_string_to_host("127.0.0.1", "8080", addr);
  assert(!err);
}

void test_invalid_ip() {
    sockaddr_in addr{};
    auto err = convert_string_to_host("256.256.256.256", "8080", addr);
    assert(err.has_value());
    assert(err->get_err_message() == "Invalid host");
}

void test_invalid_port_zero() {
    sockaddr_in addr{};
    auto err = convert_string_to_host("127.0.0.1", "0", addr);
    assert(err.has_value());
    assert(err->get_err_message() == "Invalid port");
}

void test_invalid_port_too_large() {
    sockaddr_in addr{};
    auto err = convert_string_to_host("127.0.0.1", "70000", addr);
    assert(err.has_value());
    assert(err->get_err_message() == "Invalid port");
}

void statistic_test() {
  Statistic stats;

  // Проверяем начальное состояние
  assert(stats.get_count_message() == 0);

  // Создаем протокол с сообщением уровня INFO
  auto msg1 = std::make_shared<std::string>("Test message 1");
  time_t now = std::time(nullptr);
  Logger::Logger_protocol::Protocol entry1(msg1, Logger::Level::INFO, now);

  stats.update(entry1);

  assert(stats.get_count_message() == 1);
  auto data = stats.get_statistics_data();
  assert(data.Level_INFO_count == 1);
  assert(data.Level_WARN_count == 0);
  assert(data.Level_ERROR_count == 0);
  assert(data.all_count == 1);
  assert(data.count_last_interval_time == 1);
  assert(data.max_length == msg1->size());
  assert(data.min_length == msg1->size());
  assert(data.averege_length == msg1->size());

  // Добавляем сообщение WARN
  auto msg2 = std::make_shared<std::string>("Warn message");
  Logger::Logger_protocol::Protocol entry2(msg2, Logger::Level::WARN, now + 1);

  stats.update(entry2);

  assert(stats.get_count_message() == 2);
  data = stats.get_statistics_data();
  assert(data.Level_INFO_count == 1);
  assert(data.Level_WARN_count == 1);
  assert(data.Level_ERROR_count == 0);
  assert(data.all_count == 2);
  assert(data.count_last_interval_time == 2);
  assert(data.max_length == std::max(msg1->size(), msg2->size()));
  assert(data.min_length == std::min(msg1->size(), msg2->size()));
  assert(data.averege_length == (msg1->size() + msg2->size()) / 2);

  // Добавляем ERROR с большим временем, чтобы проверить удаление старых отметок времени
  auto msg3 = std::make_shared<std::string>("Error message with some length");
  Logger::Logger_protocol::Protocol entry3(msg3, Logger::Level::ERROR, now + Statistic::interval_time + 1);

  stats.update(entry3);

  data = stats.get_statistics_data();
  // Старые временные метки должны удаляться
  assert(data.count_last_interval_time == 1);

  // Проверяем правильность счетчиков по уровням
  assert(data.Level_INFO_count == 1);
  assert(data.Level_WARN_count == 1);
  assert(data.Level_ERROR_count == 1);

  // Проверяем длину
  assert(data.max_length == std::max({msg1->size(),msg2->size(),msg3->size()}));
  assert(data.min_length == std::min({msg1->size(),msg2->size(),msg3->size()}));
  uint64_t sum_len = msg1->size() + msg2->size() + msg3->size();
  assert(data.averege_length == sum_len / 3);
}

int main() {
  test_valid_ip_port();
  test_invalid_ip();
  test_invalid_port_zero();
  test_invalid_port_too_large();
  statistic_test();
  return 0;
}
