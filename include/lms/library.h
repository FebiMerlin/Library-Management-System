#pragma once

#include <string>
#include <vector>

#include "lms/book.h"
#include "lms/config.h"
#include "lms/result.h"
#include "lms/student.h"
#include "lms/transaction.h"

namespace lms {

// Core business logic. Contains no I/O so it can be unit-tested; the console
// UI lives in src/cli. Replaces the global arrays of the original program.
class Library {
public:
    explicit Library(Config config = {});

    const Config& config() const { return config_; }

    // Current date, in days since epoch. The CLI sets it from the system
    // clock; tests set it explicitly to make fines deterministic.
    Day today() const { return today_; }
    void setToday(Day day) { today_ = day; }

    // ---- Books (admin) ----
    Result addBook(int isbn, const std::string& title, const std::string& author);
    Result editBook(int isbn, const std::string& title, const std::string& author);
    Result removeBook(int isbn);
    const Book* findBook(int isbn) const;
    const std::vector<Book>& books() const { return books_; }
    // Case-insensitive substring search over title, author and ISBN.
    std::vector<Book> searchBooks(const std::string& query, bool onlyAvailable = false) const;
    std::vector<Book> overdueBooks() const;
    int daysOverdue(const Book& book) const;

    // ---- Students ----
    Result createAccount(int roll, const std::string& name, Money initialDeposit);
    Result closeAccount(int roll, Money* refunded = nullptr);
    Result deposit(int roll, Money amount);
    const Student* findStudent(int roll) const;
    const std::vector<Student>& students() const { return students_; }
    std::vector<Student> studentsSortedByRoll() const;

    // ---- Loans ----
    Result issueBook(int roll, int isbn);
    // On success `fine` (if given) receives the late fee charged (0 if on time).
    Result returnBook(int roll, int isbn, Money* fine = nullptr);
    std::vector<Book> booksIssuedTo(int roll) const;

    // ---- History ----
    const std::vector<Transaction>& transactions() const { return transactions_; }
    std::vector<Transaction> history(int roll) const;

    // Used by storage to rebuild state without re-applying business rules.
    void restore(std::vector<Book> books, std::vector<Student> students, std::vector<Transaction> transactions);

    static Result validateName(const std::string& name);
    static Result validateRoll(int roll);
    static Result validateIsbn(int isbn);

private:
    Book* findBookMut(int isbn);
    Student* findStudentMut(int roll);
    void record(const Student& s, TxType type, Money amount, int isbn = 0);

    Config config_;
    Day today_ = 0;
    std::vector<Book> books_;
    std::vector<Student> students_;
    std::vector<Transaction> transactions_;
};

}  // namespace lms
