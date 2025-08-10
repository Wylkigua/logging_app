#include <cassert>
#include <string>
#include <thread>
#include <vector>
#include "../src/logger_app.hpp"


void test_send_receive() {
  Channel ch;
  std::vector<std::string> data{"message 1", "message 2", "message 3"};
  int sent_count = data.size();
  auto sender = std::thread([&]{
      for (int i = 0; i < sent_count; ++i) {
          auto msg = std::make_shared<std::string>(data[i]);
          bool ok = ch.send(msg, time(nullptr));
          assert(ok);
      }
      // закрываем канал - уведомляем получателя
      ch.notify_error_receiver();
  });
  auto receiver = std::thread([&]{
      for(int i = 0; i < sent_count; ++i) {
          // ожидаем данных либо закрытия канала
          auto msg = ch.receive_wait();
          if (!msg) break; // канал закрыт
          assert(*msg.value().message == data[i]);
      }
  });
  sender.join();
  receiver.join();
}

void test_non_blocking_receive() {
  Channel ch;
  auto msg = ch.receive_not_wait();
  assert(!msg.has_value()); // должно быть пусто
  bool ok = ch.send(std::make_shared<std::string>("Hi"), time(nullptr));
  assert(ok);
  msg = ch.receive_not_wait();
  assert(msg.has_value());
  assert(*msg->message == "Hi");
}

void test_close_receive() {
  Channel ch;
  // получатель закрыл канал
  ch.notify_error_sender();
  bool ok = ch.send(std::make_shared<std::string>("This won't be sent"), time(nullptr));
  assert(!ok);
}

int main() {
  test_send_receive();
  test_non_blocking_receive();
  test_close_receive();
}
