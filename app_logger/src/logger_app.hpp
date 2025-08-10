#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <atomic>
#include <optional>

#include "logger.hpp"

/**
 * @brief Структура, для записи сообщения в канал.
 */
struct Chanel_protocol {
  std::shared_ptr<std::string> message; ///< Сообщение
  time_t time; ///< Метка времени сообщения
};

/**
 * @brief Потокобезопасный односторонний канал для передачи сообщений между потоками
 *
 * Channel обеспечивает блокирующую и неблокирующую выборку сообщений из очереди
 * Получатель и отправитель могут сигнализировать об ошибках через канал
 */
class Channel {
  std::queue<Chanel_protocol> data; ///< Очередь сообщений
  std::mutex mtx; ///< Мьютекс для защиты очереди
  std::condition_variable condvar; ///< Условная переменная для ожидания сообщений
  std::atomic<bool> close_sender{false}; ///< Флаг закрытия отправителя
  std::atomic<bool> close_receive{false}; ///< Флаг закрытия получателя
public:
  Channel() = default;
  Channel(const Channel&) = delete;
  Channel& operator=(const Channel&) = delete;

  void notify_error_receiver();
  void notify_error_sender();
  bool send(const std::shared_ptr<std::string>, const time_t);
  std::optional<Chanel_protocol> receive_wait();
  std::optional<Chanel_protocol>receive_not_wait();
};

void write_logging_file(const std::string& file,
  const Logger::Level level, Channel& channel);
