#pragma once

#include <string>
#include <vector>

#include "lms/library.h"

#include "console.h"

namespace lms::cli {

struct Options {
    std::string dataFile = "library.dat";
    std::string adminPassword;  // empty -> taken from LMS_ADMIN_PASSWORD or default
    bool noSave = false;        // used by CTest smoke tests
};

class App {
public:
    App(Library& library, Console& console, Options options);

    // Runs the interactive session; returns the process exit code.
    int run();

private:
    // Menus
    bool mainMenu();
    void adminSession();
    void studentSession();

    // Admin actions
    void addBook();
    void editBook();
    void removeBook();
    void listBooks(const std::vector<Book>& books);
    void searchBooks();
    void listStudents();
    void viewStudent();
    void overdueReport();
    void closeAccount();

    // Student actions
    bool createAccount(int roll);
    void showAccount(int roll);
    void depositMoney(int roll);
    void issueBook(int roll);
    void returnBook(int roll);
    void showStatement(int roll);

    void report(const Result& r, const std::string& successMessage);
    void persist();

    Library& lib_;
    Console& con_;
    Options opt_;
};

}  // namespace lms::cli
