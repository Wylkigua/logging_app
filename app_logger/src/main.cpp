#include <iostream>
#include <string>
#include <thread>
#include "logger_app.hpp"

int main(const int argc, char const *argv[]) {
  if (argc < 3) {
    std::cout << "using <file logging> <LEVEL message>";
    return 1;
  }
  std::string file(argv[1]);
  std::string level(argv[2]);
  auto log_level = Logger::deserialization_level(level);
  if (!log_level) {
    std::cout << "level: " <<
    Logger::serialization_level(Logger::Level::INFO).value() << " " <<
    Logger::serialization_level(Logger::Level::WARN).value() << " " <<
    Logger::serialization_level(Logger::Level::ERROR).value() << std::endl;
  }
  Channel channel;
  /* создаем поток и передаем данные для инициализации логирования
     и ссылку на канал для обмена сообщениями
  */
  std::thread thread_logging([&file, &log_level, &channel]{
    write_logging_file(file, log_level.value(), channel);
  });

  std::string line;
  while (std::getline(std::cin, line)) {
    /* если поток в состоянии ошибки
       уведомляем получателя и закрываем канал */
    if (std::cin.fail()) {
      channel.notify_error_receiver();
      break;
    }
    /* если получатль закрыл канал - выходим из цикла
       иначе передаем в канал сообщение и текущее время
      */
    if (!channel.send(std::make_shared<std::string>(std::move(line)),
      std::time(nullptr))) {
      break;
    }
    line.clear();
  }
  thread_logging.join();
  return 0;
}
