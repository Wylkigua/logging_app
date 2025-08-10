#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <fstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <variant>

/**
 * @file logger.hpp
 * @brief Заголовочный файл модуля логирования
 *
 * Содержит определения классов, структур и функций для
 * организации записи логов в файл или в сокет
 */

namespace Logger {

  /**
   * @enum Level
   * @brief Уровни важности сообщений лога
   */
   enum class Level {
     INFO,  ///< Информационные сообщения
     WARN,  ///< Предупреждения
     ERROR  ///< Ошибки
   };

  /**
   * @enum Error_code
   * @brief Коды ошибок, возвращаемые методами логгера
   */
   enum class Error_code {
     ERROR,         ///< Ошибка
     OPEN_SESSION,  ///< Ошибка при открытии сессии
     CLOSE_SESSION, ///< Ошибка при закрытии сессии
     WRITE          ///< Ошибка при записи сообщения
   };

  namespace Logger_protocol {
    /**
     * @class Protocol
     * @brief Класс для хранения и формирования записи лога
     *
     * Содержит сообщение, уровень логирования и время секундах
     */
    class Protocol {
      std::shared_ptr<std::string> message; ///< Сообщение
      Level level{}; ///< Уровень
      time_t time{}; ///< Время секунды
      public:
      /**
       * @brief Конструктор протокола лога.
       *
       * @param msg Указатель на строку с сообщением.
       * @param lvl Уровень логирования (enum Level).
       * @param t Unix Метка времени в секундах.
       */
      Protocol(
        const std::shared_ptr<std::string>& msg,
        const Level lvl,
        time_t t
      ) : message(msg), level(lvl), time(t) {}
      Protocol() = default;
      std::optional<Protocol>
      create_log_entry(std::string&&, const Level, time_t);
      Level get_level() const { return level; }
      time_t get_time() const { return time; }
      std::shared_ptr<std::string>
      get_message() const { return message; }
    };

    template<typename T>
    std::optional<T> extract_last_number(std::string&);

    std::shared_ptr<std::string> serialization_log(const Protocol&);
    std::optional<Protocol> deserialization_log(std::shared_ptr<std::string>);
    std::ostream& print_log_entry(std::ostream& os, const Protocol&);
  }

  struct Error {
    Error_code code{};      ///< Код ошибки
    std::string error_message; ///< Сообщение об ошибке
    public:
    Error(const Error_code code, const std::string& msg = "")
      : code(code), error_message(msg)
    {}
    Error_code get_code() const { return code; }
    std::string get_err_message() const { return error_message; }
  };

  /**
   * @class Session
   * @brief Абстрактный интерфейс сессии логирования
   */
  class Session {
    public:
    Session() = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /// Открывает сессию
    virtual std::optional<Error> open_session() = 0;
    /// Закрывает сессию
    virtual std::optional<Error> close_session() = 0;
    /// Записывает сообщение
    virtual std::optional<Error>
    write(const Logger_protocol::Protocol&) = 0;
    virtual ~Session() = default;
  };

  /**
   * @class Logging
   * @brief Основной интерфейс логгера, позволяющий записывать сообщения
   *        в различные источники (файл или сокет)
   *
   * Класс реализует обёртку над сессией записи логов (Session) и предоставляет
   * методы открытия/закрытия сессии, а также записи лог-сообщений с заданным уровнем
   */
  class Logging {
    std::unique_ptr<Session> session; ///< Объект сессии (файл или сокет)
    std::atomic<Level> level; ///< Минимальный уровень логирования
    Logger_protocol::Protocol protocol; ///< Протокол формирования лог-записей

    public:
    /// Конструктор для записи в сокет
    Logging(const std::string& host,const std::string& port,Level level);
    /// Конструктор для записи в файл
    Logging(const std::string& file_name, Level level);

    Logging() = delete;
    Logging(const Logging&) = delete;
    Logging& operator=(const Logging&) = delete;

    std::optional<Error> open_session();
    std::optional<Error> close_session();

    std::optional<Error>
    log_write(std::shared_ptr<std::string>, time_t);

    void set_level(const Level);
  };

  /**
   * @class Socket_logging
   * @brief Реализация сессии логирования через сокет
   * @brief Использует протокол IPv4 и TCP
   */
  class Socket_logging final : public Session {
    friend class Logging;
    int fd{};
    std::string host, port;

    Socket_logging(const std::string& host, const std::string& port)
      : host(host), port(port)
    {}
    public:
    Socket_logging(const Socket_logging&) = delete;
    Socket_logging& operator=(Socket_logging&) = delete;
    ~Socket_logging() override { close_session(); }
    private:
    std::optional<Error> open_session() override;
    std::optional<Error> close_session() override;
    std::optional<Error> write(const Logger_protocol::Protocol&) override;
  };

  /**
   * @class File_logging
   * @brief Реализация сессии логирования в файл.
   */
  class File_logging final: public Session {
    friend class Logging;
    std::ofstream log_file;
    std::string file_name;

    File_logging(const std::string& file_name) : file_name(file_name) {}
    File_logging(const File_logging&) = delete;
    File_logging& operator=(const File_logging&) = delete;

    public:
    ~File_logging() override { close_session(); }

    private:
    std::optional<Error> open_session() override;
    std::optional<Error> close_session() override;
    std::optional<Error> write(const Logger_protocol::Protocol&) override;
  };

  std::optional<Level> deserialization_level(const std::string&);
  std::optional<std::string>serialization_level(const Level);

  namespace Socket {
    /// читает сокет
    std::variant<std::shared_ptr<std::string>, Error> socket_read(const int);
    /// пишет в сокет
    std::variant<int, Error> socket_write(const int,std::shared_ptr<std::string>);
  }
}
