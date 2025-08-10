#include "logger.hpp"
#include <chrono>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#define LISTEN_QUEUE 512

enum class Error_code {
  ERROR
};

struct Error {
  Error_code code{};
  std::string error_message;
  public:
  Error(const Error_code code, const std::string& msg = "")
    : code(code), error_message(msg)
  {}
  Error_code get_code() const { return code; }
  std::string get_err_message() const { return error_message; }
};

struct Statistics_data {
  uint64_t sum_length{}, averege_length{};
  uint64_t max_length{}, min_length{};
  int Level_INFO_count{}, Level_WARN_count{},Level_ERROR_count{};
  int count_last_interval_time{}, all_count;
};

/**
 * @class Statistic
 * @brief Класс для сбора и отображения статистики лог-сообщений за заданный интервал времени.
 */
class Statistic {
  std::unordered_map<Logger::Level,int> level_map{
    {Logger::Level::INFO, 0},
    {Logger::Level::WARN, 0},
    {Logger::Level::ERROR, 0}
  };
  uint64_t sum_length{}, averege_length{};
  uint64_t max_length{};
  uint64_t min_length = std::numeric_limits<uint64_t>::max();
  std::queue<time_t> times{};
  public:
  static constexpr int interval_time = 3600; // 1 час
  std::ostream& statistic_display(std::ostream& os) const;
  Statistics_data get_statistics_data() const;
  void update(const Logger::Logger_protocol::Protocol&);
  uint64_t get_count_message() const;
  private:
  void add_time(time_t);
  void update_length_message(uint64_t);
};

int statistic_app_run(const int, const std::chrono::seconds, const int);
std::variant<int, Error>
init_listen_server(const std::string&, const std::string&);

std::optional<Error>
convert_string_to_host(const std::string&, const std::string&, sockaddr_in&);
