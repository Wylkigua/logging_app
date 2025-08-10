#include "statistic_app.hpp"
#include <sys/poll.h>
#include <optional>
#include <iostream>
#include <arpa/inet.h>
#include <variant>

/**
 * @brief Выводит статистику сообщений в поток.
 * @param os Поток вывода.
 * @return ostream& Возвращает ссылку на поток.
 */
std::ostream&
Statistic::statistic_display(std::ostream& os) const {
  os << "Message statistic:" << '\n' <<
  "count: " << get_count_message() << '\n' <<
  "level " << Logger::serialization_level(Logger::Level::INFO).value() << ":" <<
  level_map.at(Logger::Level::INFO) << '\n' <<
  "level " << Logger::serialization_level(Logger::Level::WARN).value() << ":" <<
  level_map.at(Logger::Level::WARN) << '\n' <<
  "level " << Logger::serialization_level(Logger::Level::ERROR).value() << ":" <<
  level_map.at(Logger::Level::ERROR) << '\n' <<
  "last hour: " << times.size() << '\n' <<
  "max length: " << max_length << '\n' <<
  "min length: " << min_length << '\n' <<
  "averege length: " << averege_length;
  return os;
}

Statistics_data
Statistic::get_statistics_data() const {
  Statistics_data data;
  data.Level_INFO_count = level_map.at(Logger::Level::INFO);
  data.Level_WARN_count = level_map.at(Logger::Level::WARN);
  data.Level_ERROR_count = level_map.at(Logger::Level::ERROR);
  data.all_count = get_count_message();
  data.count_last_interval_time = times.size();
  data.averege_length = averege_length;
  data.max_length = max_length;
  data.min_length = min_length;
  data.sum_length = sum_length;
  return data;
}

/**
 * @brief Обновляет статистику с учётом нового лог-сообщения.
 *
 * Увеличивает счётчик сообщений для соответствующего уровня,
 * обновляет статистику длины сообщений и временные метки.
 *
 * @param entry_log Объект Protocol с информацией о лог-сообщении.
 */
void Statistic::update(const Logger::Logger_protocol::Protocol& entry_log) {
  ++level_map[entry_log.get_level()];
  update_length_message(entry_log.get_message()->size());
  add_time(entry_log.get_time());
}

/**
 * @brief Возвращает общее количество обработанных сообщений.
 * Суммирует количество сообщений по всем уровням логирования.
 * @return uint64_t Общее количество сообщений.
 */
uint64_t Statistic::get_count_message() const {
  return (
    level_map.at(Logger::Level::INFO) +
    level_map.at(Logger::Level::WARN) +
    level_map.at(Logger::Level::ERROR));
}

/**
 * @brief Добавляет отметку времени в статистику и удаляет устаревшие записи.
 *
 * @param time Временная метка (в формате time_t), которая добавляется в очередь.
 * @note По протоколу логирования время приходит в секундах
 *       class Statistic хранит interval равным 3600 секунд = 1 час
 * @details
 * Функция помещает новое значение времени в очередь.
 * Затем проверяет разницу между последним и первым элементом очереди.
 * Если эта разница больше или равна заданному интервалу времени, то
 * удаляет устаревшие временные метки из начала очереди до тех пор,
 * пока интервал между крайними элементами не станет меньше заданного значения.
 */
void Statistic::add_time(time_t time) {
  times.push(time);
  while(times.back() - times.front() >= interval_time) {
    times.pop();
  }
}

/**
 * @brief Обновляет статистику по длинам сообщений.
 *
 * Пересчитывает сумму, максимальную, минимальную и среднюю длины сообщений.
 * Средняя длина пересчитывается на основе текущего количества сообщений.
 *
 * @param uint64_t length Длина нового сообщения.
 */
void Statistic::update_length_message(uint64_t length) {
  sum_length += length;
  max_length = std::max(max_length, length);
  min_length = std::min(min_length, length);
  int count = !get_count_message() ? 1 : get_count_message();
  averege_length = sum_length / count;
}

/**
 * @brief Запускает статистическое приложение,
 *        принимающее данные по сокету и обрабатывающее их
 *        с периодическим выводом статистики.
 *
 * @param listen_fd Дескриптор слушающего сокета для принятия новых подключений.
 * @param interval_time Интервал времени (секунды) между отображением статистики.
 * @param interval_count_message Интервал сообщений (количество) между выводами статистики.
 *
 * @return Возвращает 0 при успешном завершении или -1 в случае ошибки
 *
 * @details
 * Функция для принятия подключений и чтении сообщений из сокета.
 * Использует системный вызов poll с таймаутом interval_time.
 * При получении сообщений десериализует лог-записи и выводит их.
 * Периодически, после каждых interval_count_message сообщений, выводит статистику.
 *
 * Таймаут в poll задаёт максимальное время ожидания новых данных,
 * если он истекает то статистика выводится на экран,
 * при условии что она изменилась с последнего вывода
 *
 * Aлгоритм отображения статистики:
 * - запоминаем количество сообщений при последнем выводе статистики.
 * - после истечения таймаута сравниваем текущее количество сообщений
 *   с количесвтом последнего отображения, если они не равны данные статистики обновились
 */
int statistic_app_run(
  const int listen_fd,
  std::chrono::seconds interval_time,
  const int interval_count_message
) {
  //конвертируем в млс для poll
  int timeout = std::chrono::duration_cast<std::chrono::milliseconds>(interval_time).count();
  Statistic stats;
  const int tracket_length = 1;
  pollfd tracket_set_fd[tracket_length];
  auto& tracket_fd = tracket_set_fd[0];
  int previous_count_message{}; // для отслеживания изменений в статистике
  while (true) {
    int new_connect_fd = ::accept(listen_fd,nullptr,nullptr);
    if (new_connect_fd == -1) {
      if (errno == EINTR) continue;
      std::cerr << strerror(errno) << std::endl;
      return -1;
    }
    tracket_fd.fd = new_connect_fd;
    tracket_fd.events = POLLIN | POLLRDHUP;
    while (true) {
      int result_tracket = poll(tracket_set_fd, tracket_length, timeout);
      if (result_tracket == -1) {
        if (errno == EINTR) continue;
        std::cerr << strerror(errno) << std::endl;
        close(tracket_fd.fd);
        break;
      }
      // пришли новые данные из сокета
      if (result_tracket == 1 && tracket_fd.revents | POLLIN) {
        auto data_socket = Logger::Socket::socket_read(tracket_fd.fd);
        if (auto message =
            std::get_if<std::shared_ptr<std::string>>(&data_socket)) {
          if (auto log_entry = Logger::Logger_protocol::deserialization_log(*message)) {
            Logger::Logger_protocol::print_log_entry(std::cout, log_entry.value()) << std::endl;

            // обновляем статистику
            stats.update(log_entry.value());
            /*
              если достиг интервал сообщений то выводим статистику
              и обновляем количество сообщений при последнем выводе
              текущим количеством сообщений
            */
            if (!(stats.get_count_message() % interval_count_message)) {
              previous_count_message = stats.get_count_message();
              stats.statistic_display(std::cout) << std::endl;
            }
          }
        } else {
          auto error = std::get<Logger::Error>(data_socket);
          std::cout << error.get_err_message() << std::endl;
          close(tracket_fd.fd);
          break;
        }
      }
      /* если истек таймаут,
         то проверяем количество сообщений при последнем отображении,
         если оно изменилось то выводим статистику и обновляем количество
         сообщений при последнем выводе текущим количеством сообщений
      */
      if (!result_tracket && stats.get_count_message() > previous_count_message) {
        previous_count_message = stats.get_count_message();
        stats.statistic_display(std::cout) << std::endl;
      }
    }
  }
  return 0;
}

/**
 * @brief Инициализирует TCP-сокет и переводит его в режим прослушивания.
 * @param host IPv4-адрес в строковом виде.
 * @param port Порт в строковом виде.
 *
 * @return variant<int, Error>
 *         - int  — файловый дескриптор слушающего сокета.
 *         - Error — ошибка с кодом и сообщением.
 */
std::variant<int, Error>
init_listen_server(const std::string& host, const std::string& port) {
  sockaddr_in listen_address{};
  int fd{};
  if (auto error = convert_string_to_host(host,port,listen_address)) {
    return error.value();
  }
  if ((fd = ::socket(AF_INET,SOCK_STREAM,0)) < 0) {
    return Error(Error_code::ERROR, strerror(errno));
  }
  int opt = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    return Error(Error_code::ERROR, strerror(errno));
  }
  if (::bind(fd, reinterpret_cast<sockaddr*>(&listen_address), sizeof(listen_address)) < 0) {
    return Error(Error_code::ERROR, strerror(errno));
  }
  if (::listen(fd, LISTEN_QUEUE) < 0) {
    return Error(Error_code::ERROR, strerror(errno));
  }
  return fd;
}

/**
 * @brief Конвертирует строковые представления IP-адреса и порта в структуру sockaddr_in.
 *
 * @param host Строка с IP-адресом в формате IPv4.
 * @param port Строка с номером порта.
 * @param[out] address Структура sockaddr_in, которую нужно заполнить результатом конвертации.
 *
 * @return optional<Error> Возвращает пустой optional при успешном преобразовании.
 *         В случае ошибки возвращает объект Error с соответствующим кодом и сообщением.
 *
 * @details
 * - Устанавливает семейство адресов AF_INET.
 * - Использует inet_pton для конвертации IP-адреса из строки в бинарный формат.
 * - Проверяет корректность IP-адреса и номера порта.
 * - Преобразует порт в сетевой порядок байтов с помощью htons.
 *
 * @note Порт должен быть положительным числом и не превышать 65535.
 *       IP-адрес должен быть валидным IPv4-адресом.
 */
std::optional<Error>
convert_string_to_host(const std::string& host, const std::string& port,
    sockaddr_in& address) {
    address.sin_family = AF_INET;
    if (int result = ::inet_pton(AF_INET, host.data(), &address.sin_addr) <= 0) {
        if (result < 0) return Error(Error_code::ERROR, strerror(errno));
        return Error(Error_code::ERROR, "Invalid host");
    }
    int port_num = ::atoi(port.data());
    if (port_num <= 0 || port_num > std::numeric_limits<uint16_t>::max()) {
        return Error(Error_code::ERROR, "Invalid port");
    }
    address.sin_port = ::htons(static_cast<in_port_t>(port_num));
    return {};
}
