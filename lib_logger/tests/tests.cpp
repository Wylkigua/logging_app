#include "logger.hpp"

#include <cassert>
#include <ctime>
#include <string>
#include <sstream>
#include <optional>
#include <memory>
#include <iostream>


void test_create_log_entry_with_level() {
  time_t t = time(nullptr);
  Logger::Logger_protocol::Protocol prot;
  auto entry = prot.create_log_entry("Test \t message   WARN", Logger::Level::INFO, t);
  assert(entry.has_value());
  assert(entry->get_level() == Logger::Level::WARN);
  assert(*entry->get_message() == "Test message");
  assert(entry->get_time() == t);
}

void test_create_log_entry_without_level() {
  time_t t = time(nullptr);
  Logger::Logger_protocol::Protocol prot;
  auto entry = prot.create_log_entry("Only message", Logger::Level::ERROR, t);
  assert(entry.has_value());
  assert(entry->get_level() == Logger::Level::ERROR);
  assert(*entry->get_message() == "Only message");
}

void test_create_log_entry_invalid_string() {
  time_t t = time(nullptr);
  Logger::Logger_protocol::Protocol prot;
  auto entry = prot.create_log_entry("\t \t\n", Logger::Level::INFO, t);
  assert(!entry.has_value());
}

void test_extract_last_number_success() {
  std::string s = "message 9898";
  auto res = Logger::Logger_protocol::extract_last_number<long>(s);
  assert(res.has_value());
  assert(res.value() == 9898);
  assert(s == "message");
}

void test_extract_last_number_not_number() {
  std::string s = "not number";
  auto res = Logger::Logger_protocol::extract_last_number<int>(s);
  assert(!res.has_value());
  assert(s == "not number");
}

void test_extract_last_number_empty_string() {
  std::string s = "";
  auto res = Logger::Logger_protocol::extract_last_number<int>(s);
  assert(!res.has_value());
}

void test_serialization_and_deserialization() {
  time_t t = time(nullptr);
  auto msg = std::make_shared<std::string>("Hello");
  Logger::Logger_protocol::Protocol prot(msg, Logger::Level::WARN, t);
  auto serialized = Logger::Logger_protocol::serialization_log(prot);
  assert(serialized);
  auto deserialized = Logger::Logger_protocol::deserialization_log(serialized);
  assert(deserialized.has_value());
  assert(deserialized->get_level() == Logger::Level::WARN);
  assert(deserialized->get_time() == t);
  assert(*deserialized->get_message() == *msg);
}

void test_print_log_entry() {
  // фиксируем время
  std::tm tm{};
  tm.tm_year = 125; // 2025 год
  tm.tm_mon  = 7;   // Август
  tm.tm_mday = 10;
  tm.tm_hour = 15;
  tm.tm_min  = 30;
  tm.tm_sec  = 45;

  time_t fixed_time = std::mktime(&tm);
  Logger::Logger_protocol::Protocol entry(
    std::make_shared<std::string>("Hi"),
    Logger::Level::ERROR,
    fixed_time
  );
  std::ostringstream oss;
  Logger::Logger_protocol::print_log_entry(oss, entry);
  std::string out = oss.str();
  assert(out.find("Hi") != std::string::npos);
  assert(out.find("ERROR") != std::string::npos);
  assert(out.find("2025-08-10") != std::string::npos);
  assert(out.find("15:30:45") != std::string::npos);
}

void test_file_logging_write() {

  const std::string test_filename{"test_log_file.txt"};

  std::remove(test_filename.data());

  /* лог для записи в файл */
  Logger::Logging log(test_filename, Logger::Level::WARN);

  /* исходная строка для записи в файл */
  auto msg = std::make_shared<std::string>("Test log entry ERROR");
  time_t t = ::time(nullptr);

  /* создаем запись протокола логирования */
  Logger::Logger_protocol::Protocol prot;
  auto entry = prot.create_log_entry(std::move(*msg),Logger::Level::WARN, t);
  assert(entry.has_value());

  /* записываем протокол записи лога в строковый поток */
  std::ostringstream oss;
  Logger::Logger_protocol::print_log_entry(oss, entry.value());

  /* открываем сессию логирования и
     пишем в файл исходную строку */
  auto err = log.open_session();
  assert(!err.has_value());
  err = log.log_write(msg,t);
  assert(!err.has_value());
  err = log.close_session();
  assert(!err.has_value());

  /* читаем из файла строку и
     сравниваем с записью протокола логирования */
  std::ifstream ifs("test_log_file.txt");
  assert(ifs.is_open());
  std::string line;
  std::getline(ifs, line);
  std::cout << line;
  assert(line == oss.str());

  std::remove(test_filename.data());
}

int main() {
  test_create_log_entry_with_level();
  test_create_log_entry_without_level();
  test_create_log_entry_invalid_string();
  test_extract_last_number_success();
  test_extract_last_number_not_number();
  test_extract_last_number_empty_string();
  test_serialization_and_deserialization();
  test_print_log_entry();
  test_file_logging_write();
    return 0;
}
