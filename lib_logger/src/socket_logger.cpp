#include "include/logger.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <variant>

namespace Logger {
  /*** Implementation write socket***/
  /**
   * @brief Открывает сокет-сессию, устанавливая соединение с удалённым хостом
   *
   * Ищет сетевые адреса по протоколу IPv4 по заданным параметрам и в случае успеха
   * подключается по протоколу TCP
   * @return optional<Error> Возвращает пустой optional при успехе,
   * или объект Error при ошибке открытия сессии
   */
  std::optional<Error>
  Socket_logging::open_session() {
    addrinfo  hints{};
    addrinfo  *result, *rp;
    hints.ai_family = AF_INET; ///< IPv4
    hints.ai_socktype = SOCK_STREAM; ///< TCP
    hints.ai_flags = AI_NUMERICSERV | AI_NUMERICHOST; ///< Числовой хост и порт
    if (int res = ::getaddrinfo(host.data(), port.data(), &hints, &result) != 0) {
      return Error(Error_code::OPEN_SESSION, ::gai_strerror(res));
    }
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
      fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd != -1) {
        if (!::connect(fd,rp->ai_addr,rp->ai_addrlen)) {
          break;
        }
      }
    }
    freeaddrinfo(result);
    if (rp == nullptr)
      return Error(Error_code::OPEN_SESSION, ::strerror(errno));
    return {};
  }

  /**
   * @brief Закрывает сокет-сессию, закрывая соединение и дескриптор
   *
   * Выполняет shutdown и close для сокета
   * @return optional<Error> Возвращает пустой optional при успехе, или объект Error при ошибке
  */
  std::optional<Error>
  Socket_logging::close_session() {
    if (::shutdown(fd, SHUT_RDWR) || ::close(fd)) {
      return Error(Error_code::CLOSE_SESSION, strerror(errno));
    }
    return {};
  }

  /**
   * @brief Отправляет протокол лога в сокет
   *
   * Сериализует объект Protocol в строку и отправляет через сокет
   * @param entry Объект Protocol лог-запись для отправки
   * @return optional<Error> Возвращает пустой optional при успехе, или объект Error при ошибке записи
   */
  std::optional<Error>
  Socket_logging::write(const Logger_protocol::Protocol& entry) {
    auto entry_log_string = serialization_log(entry);
    auto sent_data = Socket::socket_write(fd,entry_log_string);
    if (auto error = std::get_if<Error>(&sent_data)) {
      return Error(Error_code::WRITE, error->get_err_message());
    }
    std::cout << std::get<int>(sent_data) << '\n';
    return {};
  }

  /**
   * @brief Функция для записи данных в сокет
   *
   * Сначала отправляет размер сообщения (uint32_t в сетевом порядке байт), затем сами данные
   * @param fd Дескриптор открытого сокета
   * @param data Указатель на строку с данными для отправки
   * @return variant<int, Error> Возвращает количество отправленных байт или объект ошибки
   */
  std::variant<int, Error>
  Socket::socket_write(const int fd, std::shared_ptr<std::string> data) {
    uint32_t message_size = ::htonl(static_cast<uint32_t>(data->size()));
    /// Блокируется пока не отправит размер сообщения
    int sent = ::send(fd, &message_size, sizeof(message_size), MSG_WAITALL | MSG_NOSIGNAL);
    if (sent <= 0) {
      return Error(Error_code::WRITE, strerror(errno));
    }
    /// Блокируется пока не отправит все данные
    sent = ::send(fd, data->data(), data->size(), MSG_WAITALL | MSG_NOSIGNAL);
    if (sent <= 0) {
      return Error(Error_code::WRITE, strerror(errno));
    }
    return sent;
  }

  /**
   * @brief Функция для чтения данных из сокета
   *
   * Сначала читает длину сообщения (uint32_t в сетевом порядке байт), затем само сообщение
   * @param fd Дескриптор открытого сокета
   * @return variant<shared_ptr<string>, Error> Возвращает указатель на строку с данными или объект ошибки
   */
  std::variant<std::shared_ptr<std::string>, Error>
  Socket::socket_read(const int fd) {
    uint32_t message_length{};
    /// Блокируется пока не получит размер сообщения
    int receive = ::recv(fd, &message_length, sizeof(message_length), MSG_WAITALL);
    if (receive <= 0) {
      if (!receive) {
      return Error(
        Error_code::ERROR, "closed the connection");
      }
      return Error(
        Error_code::ERROR, strerror(errno));
    }
    auto buf = std::make_shared<std::string>();
    message_length = ::ntohl(message_length);
    /// может не выделить память
    try {
        buf->resize(message_length);
    }
    catch (const std::bad_alloc& ex) {
      return Error(Error_code::ERROR, ex.what());
    }
    /// Блокируется пока не получит все данные
    receive = recv(fd, buf->data(), buf->size(), MSG_WAITALL);
    // если прочитанные данные не равны размеру сообщения
    // возвращаем ошибку
    if (receive != message_length) {
      return Error(Error_code::ERROR, "not all data received");
    }
    return buf;
  }
  /*** Implementation write socket***/
}
