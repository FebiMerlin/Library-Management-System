# Перечень тестов

Согласован PM с Dev и DevOps (задача T21). Всего **65 тестов**: 57 модульных (GoogleTest, `tests/test_*.cpp`) и 8 сквозных для консольного интерфейса (CTest, `tests/CMakeLists.txt`, `tests/scripts/`).

Запуск: тесты выполняются автоматически при `cmake --build` (цель `run_tests`). Отдельно: `ctest --preset debug` или `build/debug/tests/lms_tests --gtest_filter='Loans.*'`.

## 1. Модульные тесты ядра (`lms_core`)

### Money — деньги (`test_money.cpp`, 5)
| Тест | Проверяет |
|---|---|
| ParseWholeDollars | разбор `"0"`, `"12"`, `"100"` |
| ParseWithCents | `"12.5"`, `"12.50"`, `"0.05"`, `".99"` |
| ParseRejectsJunk | пустая строка, буквы, отрицательные, `"12."`, три знака после точки, экспонента, пробелы |
| Format | `$0.00`, `$0.05`, `$12.50`, `$1234.05`, `-$3.07` |
| NoFloatingPointDrift | 10 × 0.10 = ровно $1.00 (проблема 1.12 из ISSUES) |

### Date — даты (`test_date.cpp`, 5)
| Тест | Проверяет |
|---|---|
| FormatKnownDays | 1970-01-01, 2000-01-01, 2026-09-14, отрицательные дни |
| ParseRoundTrip | `parseDay(formatDay(d)) == d` для набора дат |
| ParseRejectsInvalid | неверный формат, 13-й месяц, 30 февраля |
| LeapYears | 2024-02-29 верно, 2023-02-29 и 1900-02-29 — нет, 2000-02-29 верно |
| CurrentDayIsReasonable | системная дата в разумном диапазоне |

### Books — книги (`test_books.cpp`, 7)
| Тест | Проверяет |
|---|---|
| AddAndFind | добавление и поиск по ISBN |
| DuplicateIsbnRejected | повторный ISBN отклоняется, каталог не меняется |
| InvalidInputRejected | ISBN ≤ 0, пустые/слишком длинные/с табуляцией названия |
| LimitAppliesOnlyWhenFull | лимит срабатывает только при реальном заполнении (**баг 1.1**) |
| Edit | редактирование; неверные данные не портят книгу |
| Remove | удаление (F4) |
| RemoveIssuedBookRejected | выданную книгу удалить нельзя (F4) |

### Students — студенты (`test_students.cpp`, 10)
| Тест | Проверяет |
|---|---|
| CreateAccountChargesFees | $100 − $20 − $30 = $50 |
| MinimumDepositEnforced | $49.99 отклоняется, $50 принимается |
| FailedCreationLeavesNoTrace | неудачное создание не оставляет данных (**баг 1.4**) |
| DuplicateRollRejected | повторный номер отклоняется |
| InvalidRollOrNameRejected | номер ≤ 0, пустое имя, перевод строки в имени |
| StudentLimit | лимит студентов из `Config` |
| Deposit | пополнение; 0 и отрицательные суммы отклоняются (**баг 1.5**) |
| SortedListDoesNotReorderStorage | сортировка не меняет исходный порядок (**баг 1.8**) |
| CloseAccountRefundsBalanceAndDeposit | возврат баланса + залога $30 (F4) |
| CloseAccountWithBooksRejected | нельзя закрыть счёт с книгами на руках (F4) |

### Loans — выдача и возврат (`test_loans.cpp`, 10)
| Тест | Проверяет |
|---|---|
| IssueChargesFeeAndMarksBook | списание $2, отметка книги, дата выдачи |
| IssueRejectsUnknownStudentOrBook | неизвестный студент/ISBN, ISBN 0 и −1 (**баг 1.2**) |
| IssueRejectsUnavailableBook | выданную книгу нельзя выдать второй раз |
| IssueRejectsInsufficientBalance | баланс < $2 |
| MaxBooksPerStudent | не более 3 книг (F2) |
| ReturnOnTimeHasNoFine | возврат в 10-й день без штрафа (F2) |
| LateReturnIsFinedPerDay | 3 дня просрочки = $3 (F2) |
| FineCanMakeBalanceNegative | штраф может увести баланс в минус (долг) (F2) |
| ReturnRejectsWrongStudent | чужую/невыданную книгу вернуть нельзя (F2) |
| OverdueReport | отчёт по просроченным книгам (F2) |

### Search — поиск (`test_search.cpp`, 6)
| Тест | Проверяет |
|---|---|
| EmptyQueryReturnsAll | пустой запрос = все книги (F3) |
| ByTitleCaseInsensitive | `c++`, `CLEAN` (F3) |
| ByAuthor | по автору (F3) |
| ByIsbnSubstring | по части ISBN (F3) |
| NoMatch | нет совпадений (F3) |
| OnlyAvailableFilter | фильтр доступных (F3) |

### Storage — хранение (`test_storage.cpp`, 9)
| Тест | Проверяет |
|---|---|
| RoundTripThroughString | сериализация → десериализация без потерь (F1) |
| SerializedFormatIsStable | точный текст формата файла (F1) |
| RoundTripThroughFile | запись и чтение файла (F1) |
| MissingFileIsNotAnError | первый запуск без файла (F1) |
| RejectsUnknownFormat | пустой файл, чужой заголовок, другая версия (F1) |
| RejectsCorruptRecords | битые записи, дубликаты; сообщение с номером строки; состояние не меняется (F1) |
| RejectsBookIssuedToUnknownStudent | ссылочная целостность (F1) |
| ToleratesWindowsLineEndings | CRLF (F1) |
| DefaultCatalogue | стартовый каталог из 15 реальных книг (проблема 2.6) |

### History — история операций (`test_history.cpp`, 5)
| Тест | Проверяет |
|---|---|
| RecordsEveryOperationInOrder | OPEN → DEPOSIT → ISSUE → RETURN → FINE с суммами и балансами (F5) |
| IsPerStudent | выписка фильтруется по студенту (F5) |
| FailedOperationsAreNotRecorded | неудачные операции не попадают в историю (F5) |
| ClosingAccountIsRecorded | закрытие счёта в истории (F5) |
| TxTypeNamesRoundTrip | имена типов операций для файла данных (F1, F5) |

## 2. Сквозные тесты консольного интерфейса (CTest, 8)

Запускают собранный `lms` с подготовленным вводом (`tests/scripts/*.in`) и проверяют, что каждая строка из `*.expect` есть в выводе. Дата фиксируется `--today 2026-09-14`, пароль — `LMS_ADMIN_PASSWORD=secret`.

| Тест | Проверяет |
|---|---|
| cli.version | `--version` печатает `lms X.Y.Z` |
| cli.help | `--help` печатает справку |
| cli.bad_option | неизвестная опция → ненулевой код возврата |
| cli.script.exit_immediately | запуск и выход |
| cli.script.wrong_password | неверный пароль администратора отклоняется |
| cli.script.invalid_input | `abc`, `7`, `-1` в меню → повторный запрос, программа не завершается (**баг 1.3**) |
| cli.script.student_flow | регистрация → выдача → просмотр → выписка → возврат |
| cli.script.persistence | данные, созданные в первом запуске, видны администратору во втором (**F1**, две сессии) |

## 3. Что не покрыто и почему

* Интерактивные подсказки и точное форматирование таблиц — проверяются вручную по `docs/ACCEPTANCE.md`; строгие сравнения вывода сделали бы тесты хрупкими.
* Поведение при нехватке места на диске / отсутствии прав на запись — проверяется вручную (сообщение `Warning: Cannot open ... for writing`).
