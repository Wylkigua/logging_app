# Logger library
## Описание
Библиотека **Logger** — библиотека логирования, поддерживает логирование в файл и отправку логов сокет через протоколы __IPv4__ и __TCP__.

## Требования
- C++17
- компилятор GCC
- Linux (Ubuntu)

## Сборка
Исходные файлы находятся в каталоге `src`
сборка `cmake`:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Тесты
Библиотека тестировалась с использованием компилятора _GCC_ _13.3.0_ на платформе _Ubuntu_ _24.04.2_ _LTS_

Тесты находятся в каталоге `tests`
сборка `cmake`:
```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Документация
Документация по библиотеке находится в каталоге `docs`.

Для её генерации использовался [Doxygen](https://www.doxygen.nl/).

## Использование

```cpp
#include "logger.hpp"

// Логирование в файл
Logger::Logging logger_file("app.log", Logger::Level::INFO);

// Логирование через TCP-сокет
Logger::Logging logger_socket("127.0.0.1", "9000", Logger::Level::WARN);

// Запись сообщения лога
logger_file.open_session();
auto msg = std::make_shared<std::string>("Тестовое сообщение");
logger_file.log_write(msg, std::time(nullptr));
logger_file.close_session();
```