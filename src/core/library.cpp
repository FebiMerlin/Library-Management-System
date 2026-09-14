#include "lms/library.h"

#include <algorithm>
#include <cctype>

namespace lms {

namespace {

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool containsCI(const std::string& haystack, const std::string& needleLower) {
    return toLower(haystack).find(needleLower) != std::string::npos;
}

}  // namespace

Library::Library(Config config) : config_(config) {}

// ---------------------------------------------------------------- validation

Result Library::validateName(const std::string& name) {
    if (name.empty()) return Result::failure("Name must not be empty.");
    if (name.size() > 100) return Result::failure("Name is too long (max 100 characters).");
    for (char c : name) {
        if (c == '\t' || c == '\n' || c == '\r') {
            return Result::failure("Name must not contain tabs or line breaks.");
        }
    }
    return Result::success();
}

Result Library::validateRoll(int roll) {
    if (roll <= 0) return Result::failure("Roll number must be a positive integer.");
    return Result::success();
}

Result Library::validateIsbn(int isbn) {
    if (isbn <= 0) return Result::failure("ISBN must be a positive integer.");
    return Result::success();
}

// --------------------------------------------------------------------- books

Book* Library::findBookMut(int isbn) {
    auto it = std::find_if(books_.begin(), books_.end(), [&](const Book& b) { return b.isbn == isbn; });
    return it == books_.end() ? nullptr : &*it;
}

const Book* Library::findBook(int isbn) const {
    return const_cast<Library*>(this)->findBookMut(isbn);
}

Result Library::addBook(int isbn, const std::string& title, const std::string& author) {
    if (auto r = validateIsbn(isbn); !r) return r;
    if (auto r = validateName(title); !r) return Result::failure("Title: " + r.error);
    if (auto r = validateName(author); !r) return Result::failure("Author: " + r.error);
    if (static_cast<int>(books_.size()) >= config_.maxBooks) {
        return Result::failure("Book limit reached. Cannot add more books.");
    }
    if (findBook(isbn)) return Result::failure("A book with this ISBN already exists.");
    books_.push_back(Book{isbn, title, author, 0, 0});
    return Result::success();
}

Result Library::editBook(int isbn, const std::string& title, const std::string& author) {
    Book* book = findBookMut(isbn);
    if (!book) return Result::failure("Book not found.");
    if (auto r = validateName(title); !r) return Result::failure("Title: " + r.error);
    if (auto r = validateName(author); !r) return Result::failure("Author: " + r.error);
    book->title = title;
    book->author = author;
    return Result::success();
}

Result Library::removeBook(int isbn) {
    auto it = std::find_if(books_.begin(), books_.end(), [&](const Book& b) { return b.isbn == isbn; });
    if (it == books_.end()) return Result::failure("Book not found.");
    if (!it->available()) return Result::failure("Book is currently issued and cannot be removed.");
    books_.erase(it);
    return Result::success();
}

std::vector<Book> Library::searchBooks(const std::string& query, bool onlyAvailable) const {
    const std::string q = toLower(query);
    std::vector<Book> out;
    for (const Book& b : books_) {
        if (onlyAvailable && !b.available()) continue;
        if (q.empty() || containsCI(b.title, q) || containsCI(b.author, q) ||
            std::to_string(b.isbn).find(q) != std::string::npos) {
            out.push_back(b);
        }
    }
    return out;
}

int Library::daysOverdue(const Book& book) const {
    if (book.available()) return 0;
    const int overdue = (today_ - book.issuedOn) - config_.loanPeriodDays;
    return overdue > 0 ? overdue : 0;
}

std::vector<Book> Library::overdueBooks() const {
    std::vector<Book> out;
    for (const Book& b : books_) {
        if (daysOverdue(b) > 0) out.push_back(b);
    }
    return out;
}

// ------------------------------------------------------------------ students

Student* Library::findStudentMut(int roll) {
    auto it = std::find_if(students_.begin(), students_.end(), [&](const Student& s) { return s.roll == roll; });
    return it == students_.end() ? nullptr : &*it;
}

const Student* Library::findStudent(int roll) const {
    return const_cast<Library*>(this)->findStudentMut(roll);
}

void Library::record(const Student& s, TxType type, Money amount, int isbn) {
    transactions_.push_back(Transaction{s.roll, today_, type, amount, s.balance, isbn});
}

Result Library::createAccount(int roll, const std::string& name, Money initialDeposit) {
    if (auto r = validateRoll(roll); !r) return r;
    if (auto r = validateName(name); !r) return r;
    if (static_cast<int>(students_.size()) >= config_.maxStudents) {
        return Result::failure("Student limit reached. Cannot create more accounts.");
    }
    if (findStudent(roll)) return Result::failure("Account already exists for this roll number.");
    if (initialDeposit < config_.minInitialDeposit) {
        return Result::failure("Initial deposit must be at least " + formatMoney(config_.minInitialDeposit) + ".");
    }
    // The original code did some of these checks *after* partially writing the
    // new student into the global arrays. Here nothing is modified until every
    // check has passed.
    Student s{roll, name, initialDeposit - config_.openingFee - config_.securityDeposit};
    students_.push_back(s);
    record(s, TxType::AccountOpened, s.balance);
    return Result::success();
}

Result Library::closeAccount(int roll, Money* refunded) {
    Student* s = findStudentMut(roll);
    if (!s) return Result::failure("Student not found.");
    if (!booksIssuedTo(roll).empty()) {
        return Result::failure("Student still has issued books. Return them first.");
    }
    const Money refund = s->balance + config_.securityDeposit;
    Student copy = *s;
    copy.balance = 0;
    record(copy, TxType::AccountClosed, -refund);
    students_.erase(
        std::remove_if(students_.begin(), students_.end(), [&](const Student& x) { return x.roll == roll; }),
        students_.end());
    if (refunded) *refunded = refund;
    return Result::success();
}

Result Library::deposit(int roll, Money amount) {
    Student* s = findStudentMut(roll);
    if (!s) return Result::failure("Student not found.");
    if (amount <= 0) return Result::failure("Deposit amount must be positive.");
    s->balance += amount;
    record(*s, TxType::Deposit, amount);
    return Result::success();
}

std::vector<Student> Library::studentsSortedByRoll() const {
    // The original sorted the global arrays in place as a side effect of
    // displaying them; this returns a sorted copy instead.
    std::vector<Student> out = students_;
    std::sort(out.begin(), out.end(), [](const Student& a, const Student& b) { return a.roll < b.roll; });
    return out;
}

// --------------------------------------------------------------------- loans

Result Library::issueBook(int roll, int isbn) {
    Student* s = findStudentMut(roll);
    if (!s) return Result::failure("Student not found.");
    Book* b = findBookMut(isbn);
    if (!b) return Result::failure("Book not found.");
    if (!b->available()) return Result::failure("Book is currently unavailable.");
    if (static_cast<int>(booksIssuedTo(roll).size()) >= config_.maxBooksPerStudent) {
        return Result::failure("Student already has the maximum number of books (" +
                               std::to_string(config_.maxBooksPerStudent) + ").");
    }
    if (s->balance < config_.issueFee) {
        return Result::failure("Insufficient balance. Issuing a book costs " + formatMoney(config_.issueFee) + ".");
    }
    s->balance -= config_.issueFee;
    b->issuedTo = roll;
    b->issuedOn = today_;
    record(*s, TxType::BookIssued, -config_.issueFee, isbn);
    return Result::success();
}

Result Library::returnBook(int roll, int isbn, Money* fine) {
    Student* s = findStudentMut(roll);
    if (!s) return Result::failure("Student not found.");
    Book* b = findBookMut(isbn);
    if (!b) return Result::failure("Book not found.");
    if (b->issuedTo != roll) return Result::failure("This book was not issued to this student.");

    const Money lateFee = static_cast<Money>(daysOverdue(*b)) * config_.finePerDay;
    b->issuedTo = 0;
    b->issuedOn = 0;
    record(*s, TxType::BookReturned, 0, isbn);
    if (lateFee > 0) {
        // Balance may go negative: the fine is a debt, covered by the security
        // deposit on account closure.
        s->balance -= lateFee;
        record(*s, TxType::Fine, -lateFee, isbn);
    }
    if (fine) *fine = lateFee;
    return Result::success();
}

std::vector<Book> Library::booksIssuedTo(int roll) const {
    std::vector<Book> out;
    for (const Book& b : books_) {
        if (b.issuedTo == roll) out.push_back(b);
    }
    return out;
}

// ------------------------------------------------------------------- history

std::vector<Transaction> Library::history(int roll) const {
    std::vector<Transaction> out;
    for (const Transaction& t : transactions_) {
        if (t.roll == roll) out.push_back(t);
    }
    return out;
}

void Library::restore(std::vector<Book> books, std::vector<Student> students, std::vector<Transaction> transactions) {
    books_ = std::move(books);
    students_ = std::move(students);
    transactions_ = std::move(transactions);
}

}  // namespace lms
