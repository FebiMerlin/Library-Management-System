# Library Management System

[![CI](https://github.com/FebiMerlin/Library-Management-System/actions/workflows/ci.yml/badge.svg)](https://github.com/FebiMerlin/Library-Management-System/actions/workflows/ci.yml)

Консольная система учёта библиотеки на C++17: книги, счета студентов, выдача и возврат книг со штрафами за просрочку, поиск, история операций, хранение данных в файле.

Учебный проект по практической работе №1: взят «маленький некачественный проект с GitHub», проведена ревизия, добавлены сборка CMake, тесты, CI и пять новых функций. Документация по работе — в каталоге [`docs/`](docs/).

## Команда

| Роль | Участник |
|---|---|
| Менеджер проекта (PM) | Шарыгин |
| Инженер DevOps | Хребитщев |
| Разработчик (Dev) | Хамидуллин |

Трекер задач — [GitHub Projects](https://github.com/FebiMerlin/Library-Management-System/projects) этого репозитория; CI — GitHub Actions.

## Происхождение

Форк [abdulsamie10/Library-Management-System](https://github.com/abdulsamie10/Library-Management-System) (автор Abdul Samie, 2023). Исходный `main.cpp` сохранён как [`docs/original/main.cpp.orig`](docs/original/main.cpp.orig); история автора сохранена в git. Результаты ревизии исходного кода — [`docs/ISSUES.md`](docs/ISSUES.md). Исходный репозиторий не содержит файла лицензии; используется как учебный форк с указанием авторства.

## Возможности

**Администратор** (пароль из `LMS_ADMIN_PASSWORD`): добавить / изменить / удалить книгу, каталог, поиск, список студентов, счёт и выписка студента, отчёт о просроченных книгах, закрытие счёта.

**Студент** (по номеру зачётки): регистрация ($20 открытие + $30 возвращаемый залог, минимум $50), баланс, пополнение, поиск и выдача книги ($2 на 10 дней, не более 3 книг), возврат (штраф $1/день просрочки), выписка по счёту.

Подробно — [`docs/FEATURES.md`](docs/FEATURES.md).

## Сборка

Нужны: CMake ≥ 3.21 (для presets; сам проект — ≥ 3.16), компилятор C++17 (GCC 9+, Clang 10+, MSVC 2022), Ninja (для presets `debug`/`release`), интернет при первой конфигурации (скачивается GoogleTest).

```bash
cmake --preset release
cmake --build --preset release
```

Тесты выполняются **в процессе сборки**: упавший тест — упавшая сборка. Отключить: `cmake --preset release -DLMS_RUN_TESTS_ON_BUILD=OFF`; не собирать тесты вовсе: `-DLMS_BUILD_TESTS=OFF`.

Другие presets: `debug`, `ci` (предупреждения = ошибки), `msvc` (Visual Studio 2022). Без presets:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Результат: `build/release/lms` (Windows: `lms.exe`).

### Тесты отдельно

```bash
ctest --preset release
build/release/tests/lms_tests --gtest_filter='Storage.*'
```

Перечень тестов — [`docs/TESTS.md`](docs/TESTS.md).

## Запуск

```bash
export LMS_ADMIN_PASSWORD=mysecret      # PowerShell: $env:LMS_ADMIN_PASSWORD="mysecret"
build/release/lms
```

| Опция | Назначение |
|---|---|
| `--data <file>` | файл данных (по умолчанию `library.dat` в текущем каталоге) |
| `--today YYYY-MM-DD` | считать сегодняшней указанную дату (демонстрация штрафов) |
| `--no-save` | не записывать файл данных |
| `--version`, `--help` | версия / справка |

При первом запуске (нет файла данных) библиотека заполняется каталогом из 15 книг. Если `LMS_ADMIN_PASSWORD` не задан, используется пароль `admin` и печатается предупреждение.

Пример сессии:

```
Login as:
  1. Admin
  2. Student
  0. Exit
> 2
Roll number: 230001
Student not found. Create an account? (y/n): y
Name: Ivan Petrov
Opening an account costs $20.00 plus a refundable security deposit of $30.00. Minimum initial deposit is $50.00.
Initial deposit: 100
Account created.

Student menu (Ivan Petrov, balance $50.00):
  1. View my account
  2. Deposit money
  3. Issue a book
  4. Return a book
  5. Account statement
  0. Log out
> 3
Search available books (empty = all): stroustrup
ISBN  Title                                        Author                      Status
-----------------------------------------------------------------------------------------------
1001  The C++ Programming Language                 Bjarne Stroustrup           available
1011  Programming: Principles and Practice Using   Bjarne Stroustrup           available
Issuing costs $2.00 for 10 days; late returns are fined $1.00 per day.
ISBN to issue (0 to cancel): 1001
Book issued. New balance: $48.00
```

## Структура

```
include/lms/     публичные заголовки ядра (Library, Book, Student, Money, Storage, Date)
src/core/        ядро — бизнес-логика без ввода/вывода (библиотека lms_core)
src/cli/         консольный интерфейс (исполняемый файл lms)
tests/           GoogleTest + сквозные CTest-скрипты
cmake/           вспомогательные модули CMake
docs/            ТЗ, журнал ревизии, план, регламент, тесты, фичи, сценарий сдачи
infra/           docker-compose для трекера / Gitea / CI-раннера / IDE, если кафедра выделит сервер
scripts/         импорт задач плана в GitHub Issues
.github/         CI (GitHub Actions)
```

## Документация проекта

| Документ | Содержание |
|---|---|
| [docs/TZ.md](docs/TZ.md) | техническое задание |
| [docs/ISSUES.md](docs/ISSUES.md) | журнал ревизии исходного кода (35 проблем) |
| [docs/INFRASTRUCTURE.md](docs/INFRASTRUCTURE.md) | выбор трекера, CI, git, IDE, ОС — с обоснованием, ресурсами и сроками |
| [docs/PLAN.md](docs/PLAN.md), [docs/tracker_tasks.csv](docs/tracker_tasks.csv) | план работ, 40 задач для трекера |
| [docs/REGULATIONS.md](docs/REGULATIONS.md) | регламент работы, правила оформления кода |
| [docs/TESTS.md](docs/TESTS.md) | перечень тестов |
| [docs/FEATURES.md](docs/FEATURES.md) | реализованные фичи, формат файла данных |
| [docs/ACCEPTANCE.md](docs/ACCEPTANCE.md) | сценарий сдачи |
| [infra/README.md](infra/README.md) | развёртывание инфраструктуры на ресурсах кафедры |

## Разработка

Форматирование — `clang-format` 23.1.1 (`pip install clang-format==23.1.1`), проверяется в CI:

```bash
clang-format -i $(git ls-files 'include/*' 'src/*' 'tests/*' | grep -E '\.(h|cpp)$')
```

Рекомендуемая IDE — VS Code с расширениями из `.vscode/extensions.json`; CLion и Visual Studio 2022 работают через `CMakePresets.json`. Правила — [`docs/REGULATIONS.md`](docs/REGULATIONS.md).
