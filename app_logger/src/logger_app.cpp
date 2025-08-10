#include "logger_app.hpp"
#include <iostream>

void write_logging_file(const std::string& file,
  const Logger::Level level, Channel& channel) {
  Logger::Logging logger(file, level);
  if (auto error = logger.open_session()) {
    std::cerr << error.value().get_err_message() << std::endl;
    // уведомляем главный поток об ошибке
    channel.notify_error_sender();
    return;
  }
  /*
   * в цикле ожидаем поступление данных в канал
   * выход и цикла возможен только,
   * если главный поток сигнализирует об ошибке.
   * Если из канала поступили данные но при записи
   * в лог произошла ошибка - завершаем поток
   */
  while (auto data_channel = channel.receive_wait()) {
    auto message = data_channel.value().message;
    auto time = data_channel.value().time;
    if (auto error = logger.log_write(message, time)) {
      std::cerr << error.value().get_err_message() << std::endl;
      channel.notify_error_sender();
      return;
    }
  }
  /*
    * Вход во второй цикл возможен только при ошибке
    * на главном потоке,в таком случае - получаем из канала
    * все данные и пишем в лог без ожидания
    *
  */
  while (auto data_channel = channel.receive_not_wait()) {
    auto message = data_channel.value().message;
    auto time = data_channel.value().time;
    if (auto error = logger.log_write(message, time)) {
      std::cerr << error.value().get_err_message() << std::endl;
      return;
    }
  }
}

/**
 * @brief Уведомляет получателя об ошибке или завершении отправки
 * После вызова метода receive_wait() будет возвращать пустой optional
 */
void Channel::notify_error_receiver() {
  close_sender = true;
  condvar.notify_one();
}

/**
 * @brief Уведомляет отправителя об ошибке или завершении получения.
 * После вызова метода send() будет возвращать false.
 */
void Channel::notify_error_sender() {
  close_receive = true;
}

/**
 * @brief Отправляет сообщение в канал.
 *
 * @param message Указатель на строку с данными.
 * @param time Метка времени сообщения.
 * @return true, если сообщение было добавлено; false, если получатель закрыт.
 */
bool Channel::send(const std::shared_ptr<std::string> message,const time_t time) {
  if (close_receive) return false;
  {
    std::unique_lock lock(mtx);
    data.push(Chanel_protocol{message, time});
  }
  /// уведомить получателя о наличие данных в очереди
  condvar.notify_one();
  return true;
}

/**
 * @brief Получает сообщение из канала, ожидая, если очередь пуста.
 *
 * @return optional<Chanel_protocol> Сообщение или пустой optional,
 *         если отправитель закрыл канал.
 */
std::optional<Chanel_protocol>
Channel::receive_wait() {
  std::unique_lock lock(mtx);

  /* ожидаем сиганала от отправителя:
     1. поступили данные в канал
     2. уведомление об ошибке
  */
  condvar.wait(lock, [this]{
    return data.size() > 0 || close_sender;
  });
  // если отправитель уведомил об ошибке выходим
  if (close_sender) return {};
  auto log_entry = data.front();
  data.pop();
  return log_entry;
}

/**
 * @brief Получает сообщение из канала без ожидания
 * @return optional<Chanel_protocol> Сообщение или пустой optional,
 *         если очередь пуста
 * @note Для ситуации когда отправитель закрыл канал,
 *       и необходимо получить оставшиеся данные из канала
 */
std::optional<Chanel_protocol>
Channel::receive_not_wait() {
  std::unique_lock lock(mtx);
  if (!data.size()) return {};
  auto log_entry = data.front();
  data.pop();
  return log_entry;
}
