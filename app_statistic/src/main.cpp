#include "statistic_app.hpp"
#include <chrono>
#include <iostream>

int main(const int argc, char const *argv[]) {
  if (argc < 4) {
    std::cerr << "using <host> <port> <interval count message> <interval time sec>" << std::endl;
    return EXIT_FAILURE;
  }
  std::string host(argv[1]);
  std::string port(argv[2]);
  int interval_count_message = ::atoi(argv[3]);
  int interval_time = ::atoi(argv[4]);
  if (interval_count_message < 1 || interval_time < 1) {
    return EXIT_FAILURE;
  }
  auto server = init_listen_server(host, port);
  if (auto error = std::get_if<Error>(&server)) {
    std::cerr << error->get_err_message() << std::endl;
    return EXIT_FAILURE;
  }
  int fd = std::get<int>(server);

  statistic_app_run(fd, std::chrono::seconds(interval_time), interval_count_message);
  close(fd);
  return EXIT_SUCCESS;
}
