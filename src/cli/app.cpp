#include "app.h"

#include <iomanip>

#include "lms/date.h"
#include "lms/storage.h"

namespace lms::cli {

App::App(Library& library, Console& console, Options options)
    : lib_(library), con_(console), opt_(std::move(options)) {}

// ------------------------------------------------------------------ helpers

void App::report(const Result& r, const std::string& successMessage) {
    if (r) {
        con_.out() << successMessage << '\n';
        persist();
    } else {
        con_.out() << "Error: " << r.error << '\n';
    }
}

void App::persist() {
    if (opt_.noSave) return;
    if (auto r = saveToFile(lib_, opt_.dataFile); !r) {
        con_.out() << "Warning: " << r.error << '\n';
    }
}

void App::listBooks(const std::vector<Book>& books) {
    if (books.empty()) {
        con_.out() << "(no books)\n";
        return;
    }
    auto& o = con_.out();
    o << std::left << std::setw(6) << "ISBN" << std::setw(45) << "Title" << std::setw(28) << "Author"
      << "Status\n"
      << std::string(95, '-') << '\n';
    for (const Book& b : books) {
        o << std::left << std::setw(6) << b.isbn << std::setw(45) << b.title.substr(0, 43) << std::setw(28)
          << b.author.substr(0, 26);
        if (b.available()) {
            o << "available";
        } else {
            o << "issued to " << b.issuedTo << " on " << formatDay(b.issuedOn);
            if (int late = lib_.daysOverdue(b); late > 0) o << ", OVERDUE " << late << " day(s)";
        }
        o << '\n';
    }
}

// --------------------------------------------------------------------- main

int App::run() {
    con_.out() << "Library Management System\n";
    while (mainMenu()) {
    }
    persist();
    con_.out() << "Goodbye.\n";
    return 0;
}

bool App::mainMenu() {
    con_.out() << "\nLogin as:\n  1. Admin\n  2. Student\n  0. Exit\n";
    auto choice = con_.readInt("> ", 0, 2);
    if (!choice) return false;
    switch (*choice) {
        case 1: adminSession(); break;
        case 2: studentSession(); break;
        default: return false;
    }
    return true;
}

// -------------------------------------------------------------------- admin

void App::adminSession() {
    auto password = con_.readLine("Admin password: ");
    if (!password) return;
    if (*password != opt_.adminPassword) {
        con_.out() << "Incorrect password.\n";
        return;
    }
    for (;;) {
        con_.out() << "\nAdmin menu:\n"
                      "  1. Add book\n"
                      "  2. Edit book\n"
                      "  3. Remove book\n"
                      "  4. View all books\n"
                      "  5. Search books\n"
                      "  6. View enrolled students\n"
                      "  7. View student account\n"
                      "  8. Overdue report\n"
                      "  9. Close student account\n"
                      "  0. Log out\n";
        auto choice = con_.readInt("> ", 0, 9);
        if (!choice || *choice == 0) return;
        switch (*choice) {
            case 1: addBook(); break;
            case 2: editBook(); break;
            case 3: removeBook(); break;
            case 4: listBooks(lib_.books()); break;
            case 5: searchBooks(); break;
            case 6: listStudents(); break;
            case 7: viewStudent(); break;
            case 8: overdueReport(); break;
            case 9: closeAccount(); break;
        }
    }
}

void App::addBook() {
    auto isbn = con_.readInt("ISBN: ", 1);
    if (!isbn) return;
    auto title = con_.readNonEmpty("Title: ");
    if (!title) return;
    auto author = con_.readNonEmpty("Author: ");
    if (!author) return;
    report(lib_.addBook(*isbn, *title, *author), "Book added.");
}

void App::editBook() {
    auto isbn = con_.readInt("ISBN of the book to edit: ", 1);
    if (!isbn) return;
    const Book* book = lib_.findBook(*isbn);
    if (!book) {
        con_.out() << "Error: Book not found.\n";
        return;
    }
    con_.out() << "Current title: " << book->title << '\n';
    auto title = con_.readLine("New title (leave empty to keep): ");
    if (!title) return;
    con_.out() << "Current author: " << book->author << '\n';
    auto author = con_.readLine("New author (leave empty to keep): ");
    if (!author) return;
    report(lib_.editBook(*isbn, title->empty() ? book->title : *title, author->empty() ? book->author : *author),
           "Book details updated.");
}

void App::removeBook() {
    auto isbn = con_.readInt("ISBN of the book to remove: ", 1);
    if (!isbn) return;
    const Book* book = lib_.findBook(*isbn);
    if (!book) {
        con_.out() << "Error: Book not found.\n";
        return;
    }
    auto sure = con_.readYesNo("Remove \"" + book->title + "\"?");
    if (!sure || !*sure) return;
    report(lib_.removeBook(*isbn), "Book removed.");
}

void App::searchBooks() {
    auto query = con_.readLine("Search (title, author or ISBN; empty = all): ");
    if (!query) return;
    auto onlyAvailable = con_.readYesNo("Only available books?");
    if (!onlyAvailable) return;
    listBooks(lib_.searchBooks(*query, *onlyAvailable));
}

void App::listStudents() {
    const auto students = lib_.studentsSortedByRoll();
    if (students.empty()) {
        con_.out() << "(no students)\n";
        return;
    }
    auto& o = con_.out();
    o << std::left << std::setw(10) << "Roll" << std::setw(30) << "Name" << std::right << std::setw(12) << "Balance"
      << "  Books\n"
      << std::string(60, '-') << '\n';
    for (const Student& s : students) {
        o << std::left << std::setw(10) << s.roll << std::setw(30) << s.name.substr(0, 28) << std::right
          << std::setw(12) << formatMoney(s.balance) << "  " << lib_.booksIssuedTo(s.roll).size() << '\n';
    }
}

void App::viewStudent() {
    auto roll = con_.readInt("Roll number: ", 1);
    if (!roll) return;
    if (!lib_.findStudent(*roll)) {
        con_.out() << "Error: Student not found.\n";
        return;
    }
    showAccount(*roll);
    showStatement(*roll);
}

void App::overdueReport() {
    const auto overdue = lib_.overdueBooks();
    con_.out() << "Today is " << formatDay(lib_.today()) << ". Overdue books: " << overdue.size() << '\n';
    listBooks(overdue);
}

void App::closeAccount() {
    auto roll = con_.readInt("Roll number of the account to close: ", 1);
    if (!roll) return;
    const Student* s = lib_.findStudent(*roll);
    if (!s) {
        con_.out() << "Error: Student not found.\n";
        return;
    }
    auto sure = con_.readYesNo("Close account of " + s->name + " (" + std::to_string(s->roll) + ")?");
    if (!sure || !*sure) return;
    Money refund = 0;
    const Result r = lib_.closeAccount(*roll, &refund);
    report(r, "Account closed. Refund to pay out: " + formatMoney(refund));
}

// ------------------------------------------------------------------ student

void App::studentSession() {
    auto roll = con_.readInt("Roll number: ", 1);
    if (!roll) return;
    if (!lib_.findStudent(*roll)) {
        auto create = con_.readYesNo("Student not found. Create an account?");
        if (!create || !*create) return;
        if (!createAccount(*roll)) return;
    }
    for (;;) {
        const Student* s = lib_.findStudent(*roll);
        if (!s) return;
        con_.out() << "\nStudent menu (" << s->name << ", balance " << formatMoney(s->balance)
                   << "):\n"
                      "  1. View my account\n"
                      "  2. Deposit money\n"
                      "  3. Issue a book\n"
                      "  4. Return a book\n"
                      "  5. Account statement\n"
                      "  0. Log out\n";
        auto choice = con_.readInt("> ", 0, 5);
        if (!choice || *choice == 0) return;
        switch (*choice) {
            case 1: showAccount(*roll); break;
            case 2: depositMoney(*roll); break;
            case 3: issueBook(*roll); break;
            case 4: returnBook(*roll); break;
            case 5: showStatement(*roll); break;
        }
    }
}

bool App::createAccount(int roll) {
    const Config& c = lib_.config();
    auto name = con_.readNonEmpty("Name: ");
    if (!name) return false;
    con_.out() << "Opening an account costs " << formatMoney(c.openingFee) << " plus a refundable security deposit of "
               << formatMoney(c.securityDeposit) << ". Minimum initial deposit is " << formatMoney(c.minInitialDeposit)
               << ".\n";
    auto deposit = con_.readMoney("Initial deposit: ");
    if (!deposit) return false;
    const Result r = lib_.createAccount(roll, *name, *deposit);
    report(r, "Account created.");
    return static_cast<bool>(r);
}

void App::showAccount(int roll) {
    const Student* s = lib_.findStudent(roll);
    if (!s) return;
    auto& o = con_.out();
    o << "Roll: " << s->roll << "\nName: " << s->name << "\nBalance: " << formatMoney(s->balance) << '\n';
    const auto books = lib_.booksIssuedTo(roll);
    o << "Issued books: " << books.size() << " of " << lib_.config().maxBooksPerStudent << '\n';
    if (!books.empty()) listBooks(books);
}

void App::depositMoney(int roll) {
    auto amount = con_.readMoney("Amount to deposit: ");
    if (!amount) return;
    const Result r = lib_.deposit(roll, *amount);
    if (r) {
        report(r, "New balance: " + formatMoney(lib_.findStudent(roll)->balance));
    } else {
        report(r, "");
    }
}

void App::issueBook(int roll) {
    auto query = con_.readLine("Search available books (empty = all): ");
    if (!query) return;
    const auto found = lib_.searchBooks(*query, /*onlyAvailable=*/true);
    listBooks(found);
    if (found.empty()) return;
    con_.out() << "Issuing costs " << formatMoney(lib_.config().issueFee) << " for " << lib_.config().loanPeriodDays
               << " days; late returns are fined " << formatMoney(lib_.config().finePerDay) << " per day.\n";
    auto isbn = con_.readInt("ISBN to issue (0 to cancel): ", 0);
    if (!isbn || *isbn == 0) return;
    // Evaluate the operation before building the message: argument evaluation
    // order is unspecified, so the balance must be read after issueBook().
    const Result r = lib_.issueBook(roll, *isbn);
    report(r, r ? "Book issued. New balance: " + formatMoney(lib_.findStudent(roll)->balance) : "");
}

void App::returnBook(int roll) {
    const auto books = lib_.booksIssuedTo(roll);
    if (books.empty()) {
        con_.out() << "You have no issued books.\n";
        return;
    }
    listBooks(books);
    auto isbn = con_.readInt("ISBN to return (0 to cancel): ", 0);
    if (!isbn || *isbn == 0) return;
    Money fine = 0;
    const Result r = lib_.returnBook(roll, *isbn, &fine);
    std::string msg = "Book returned.";
    if (r && fine > 0) msg += " Late fine charged: " + formatMoney(fine) + ".";
    if (r) msg += " Balance: " + formatMoney(lib_.findStudent(roll)->balance);
    report(r, msg);
}

void App::showStatement(int roll) {
    const auto history = lib_.history(roll);
    auto& o = con_.out();
    o << "Statement for " << roll << " (" << history.size() << " transactions):\n";
    if (history.empty()) return;
    o << std::left << std::setw(12) << "Date" << std::setw(9) << "Type" << std::right << std::setw(12) << "Amount"
      << std::setw(12) << "Balance" << "  Book\n"
      << std::string(52, '-') << '\n';
    for (const Transaction& t : history) {
        o << std::left << std::setw(12) << formatDay(t.day) << std::setw(9) << toString(t.type) << std::right
          << std::setw(12) << formatMoney(t.amount) << std::setw(12) << formatMoney(t.balanceAfter) << "  ";
        if (t.isbn != 0) {
            o << t.isbn;
            if (const Book* b = lib_.findBook(t.isbn)) o << " " << b->title;
        }
        o << '\n';
    }
}

}  // namespace lms::cli
