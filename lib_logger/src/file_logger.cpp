#include "include/logger.hpp"

namespace Logger {
  /*** Implementation write file***/

  /**
   * @brief Открывает сессию записи в файл
   *
   * Открывает файл для дозаписи (append mode)
   * Если файл не может быть открыт, возвращает Error с кодом OPEN_SESSION
   * Если файл не сущетсвует - создается новый
   * @return optional<Error> Пустое значение в случае успеха,
   *         либо объект Error с описанием ошибки
   */
  std::optional<Error>
  File_logging::open_session() {
    if (!log_file.is_open()) {
      log_file.open(file_name, std::ios::app);
    }
    if (log_file.fail()) {
      log_file.clear();
      log_file.flush();
      return Error(Error_code::OPEN_SESSION,::strerror(errno));
    }
    return {};
  }

  /**
   * @brief Закрывает сессию записи в файл
   *
   * Закрывает файловый поток
   * В случае ошибки при закрытии возвращает Error с кодом WRITE
   *
   * @return optional<Error> Пустое значение при успешном закрытии,
   *         либо объект Error с описанием ошибки
   */
  std::optional<Error>
  File_logging::close_session()  {
    log_file.close();
    if (log_file.fail()) {
      return Error(Error_code::WRITE,::strerror(errno));
    }
    return {};
  }

  /**
   * @brief Записывает протокол лога записи в файл
   *
   * Вызывает функцию вывода лог-записи в поток, добавляет перевод строки
   * Проверяет состояние потока после записи. В случае ошибки записи
   * очищает состояние потока и возвращает Error с кодом WRITE
   *
   * @param entry Объект Protocol, содержащий лог для записи
   * @return optional<Error> Пустое значение в случае успеха,
   *         либо объект Error с описанием ошибки
   */
  std::optional<Error>
  File_logging::write(const Logger_protocol::Protocol& entry)  {
    print_log_entry(log_file,entry) << std::endl;
    if (log_file.fail()) {
      log_file.clear();
      log_file.flush();
      return Error(Error_code::WRITE,::strerror(errno));
    }
    return {};
  }

  /*** Implementation write file***/
}
