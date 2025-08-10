#include "include/logger.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <unordered_map>
#include <iomanip>

namespace Logger {

/*** Interface Logger ***/
  /**
    * @brief Конструктор для логирования в сокет
    * @param host  Адрес хоста (IP)
    * @param port  Порт для подключения
    * @param level Минимальный уровень логирования (сообщения ниже будут отфильтрованы)
  */
  Logging::Logging(const std::string& host,const std::string& port, Logger::Level level)
    : session(new Socket_logging(host, port)), level(level) {}

  /**
   * @brief Конструктор для логирования в файл
   * @param file_name Путь к файлу, в который будут записываться логи
   * @param level Минимальный уровень логирования (сообщения ниже будут отфильтрованы)
  */
  Logging::Logging(const std::string& file_name, Logger::Level level)
    : session(new File_logging(file_name)), level(level) {}

  /**
   * @brief Открывает сессию логирования
   * @return std::nullopt в случае успеха или объект Error при ошибке
   */
  std::optional<Error> Logging::open_session() {
    return session->open_session();
  }
  /**
   * @brief Закрывает сессию логирования
   * @return std::nullopt в случае успеха или объект Error при ошибке
   */
  std::optional<Error> Logging::close_session() {
    return session->close_session();
  }
  /**
   * @brief Записывает сообщение в лог, если его уровень >= минимальному уровню логирования
   *
   * @param message Указатель на строку с текстом сообщения
   * @param time Метка времени (в формате time_t)
   * @return std::nullopt в случае успеха или объект Error при ошибке
   */
  std::optional<Error>
  Logging::log_write(std::shared_ptr<std::string> message, time_t time) {
    if (auto entry_log = protocol.create_log_entry(std::move(*message), level, time)) {
      if (entry_log.value().get_level() >= level) {
        return session->write(entry_log.value());
      }
    }
    return {};
  }
  /**
   * @brief Устанавливает минимальный уровень логирования
   * Сообщения с уровнем ниже установленного будут игнорироваться
   * @param lvl Новый уровень логирования
   */
   void Logging::set_level(const Level lvl) { level = lvl; }

/*** Interface Logger ***/

/*** ---------------------------------- ***/

/*** logger protocol ***/

/**
 * @brief Извлекает последнее число из строки и удаляет его
 * @brief разделяет последнее число и строку по пробелу
 *
 * @tparam T Тип числа для извлечения (int, long, double)
 * @param entry Строка, из которой извлекается число. После вызова число будет удалено из строки
 * @return optional<T> Извлечённое число, либо пустое значение, если число не найдено
 *
 * Алгоритм:
 * - Удаляются завершающие пробелы
 * - Ищется последний пробел в строке
 * - После пробела ожидается число, парсится через std::from_chars
 * - Из строки вырезается пробел и число
 * - В случае ошибки возвращается пустой std::optional
 */
template<typename T>
std::optional<T>
Logger_protocol::extract_last_number(std::string& entry) {
  if (!entry.size()) return {};
  while (std::isspace(entry.back())) entry.pop_back();
  T value{};
  auto position = entry.rfind(' '); // разделяем строку по пробелу
  if (position == std::string::npos) return {};
  // извлекаем число
  std::string_view string_value(entry.data() + position + 1);
  auto cast_data = std::from_chars(string_value.data(), string_value.data() + string_value.size(), value);
  if (cast_data.ec != std::errc()) return {};
  // вырезаем из строки найденную позцию
  entry.erase(position);
  return value;
}

/**
 * @brief Сериализует запись протокола в строку разделяя пробелами
 *
 * @param entry Объект Protocol, содержащий сообщение, уровень и время.
 * @return shared_ptr<string> Готовая строка формата "<сообщение> <уровень> <время>".
 */
std::shared_ptr<std::string>
Logger_protocol::serialization_log(const Protocol& entry) {
  std::ostringstream os;
  os << *entry.get_message() << " " <<
  static_cast<int>(entry.get_level()) << " " <<
  entry.get_time();
  return std::make_shared<std::string>(std::move(os.str()));
}

/**
 * @brief Десериализует строку в объект Protocol
 *
 * @param entry shared_ptr<string> указатель на строку формата "<сообщение> <уровень> <время>"
 * @return optional<Protocol> Объект протокола или пустое значение в случае ошибки
 *
 * Алгоритм:
 * - Извлекается время (long)
 * - Извлекается уровень (int)
 * - Остаток строки используется как сообщение
 */
std::optional<Logger_protocol::Protocol>
Logger_protocol::deserialization_log(std::shared_ptr<std::string> entry) {
  auto time = extract_last_number<long>(*entry);
  if (!time) return {};
  auto level = extract_last_number<int>(*entry);
  if (!level) return {};
  return Protocol(
    entry,
    static_cast<Level>(level.value()),
    static_cast<time_t>(time.value())
  );
}

/**
 * @brief Создаёт объект протокола из строки
 *
 * @param data Строка, содержащая сообщение и, возможно, уровень
 * @param default_level Уровень по умолчанию, если в строке нет уровня
 * @param time Временная метка записи
 * @return optional<Protocol> Готовый объект или пустое значение, если строка пустая
 *
 * @note Из строки извлекаются только не пробельные символы
 */
std::optional<Logger_protocol::Protocol>
Logger_protocol::Protocol::create_log_entry(std::string&& data, const Level default_level, time_t time) {
  std::istringstream is(std::move(data));
  std::string word;
  auto message = std::make_shared<std::string>();
  while (is >> word) {
    message->append(word);
    message->push_back(' ');
  }
  // пустая строка
  if (!word.size()) {
    return {};
  }
  message->pop_back();
  if (auto level = deserialization_level(word)) {
    // в строке находится уровень сообщения
    message->erase(
      std::find_if(message->rbegin() + word.size(), message->rend(),
        [](const char ch) {
          return !std::isspace(ch);
        }).base(),
      message->end());
    // пустая строка
    if (!message->size()) {
      return {};
    }
    return Protocol(message, level.value(), time);
  }
  return Protocol(message, default_level, time);
}

/**
 * @brief Печатает протокол лога в поток
 *
 * @param os Выходной поток
 * @param log_entry Объект Protocol
 * @return std::ostream& Ссылка на поток
 *
 * Формат: "<сообщение> <уровень> <YYYY-MM-DD HH:MM:SS>"
 */
std::ostream&
Logger_protocol::print_log_entry(std::ostream& os, const Protocol& log_entry) {
  time_t time = log_entry.get_time();
  tm tm = *std::localtime(&time);
  os << *log_entry.get_message() << " " <<
  serialization_level(log_entry.get_level()).value() << " " <<
  std::put_time(&tm, "%F %T");
  return os;
}

/*** logger protocol ***/

/**
 * @brief Преобразует строковое представление уровня в enum Level
 *
 * @param level Строка ("INFO", "WARN", "ERROR")
 * @return std::optional<Level> Уровень или пустое значение, если строка некорректна
 */
std::optional<Level> deserialization_level(const std::string& level) {
  static const std::unordered_map<std::string, Level> mmap{
    {"INFO", Level::INFO},
    {"WARN", Level::WARN},
    {"ERROR", Level::ERROR}
  };
  if (auto it = mmap.find(level); it != mmap.end()) {
    return it->second;
  }
  return {};
}

/**
 * @brief Преобразует enum Level в строку
 *
 * @param level Уровень (INFO, WARN, ERROR)
 * @return std::optional<std::string> Строковое представление или пустое значение
 */
std::optional<std::string>serialization_level(const Level level) {
  switch (level) {
    case Level::INFO: return "INFO"; break;
    case Level::WARN: return "WARN"; break;
    case Level::ERROR: return "ERROR"; break;
  }
  return {};
}

}
