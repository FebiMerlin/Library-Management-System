#include <cstdlib>
#include <iostream>
#include <string>

#include "lms/date.h"
#include "lms/library.h"
#include "lms/storage.h"
#include "lms/version.h"

#include "app.h"
#include "console.h"

namespace {

void printUsage(std::ostream& out) {
    out << "Usage: lms [options]\n"
           "\n"
           "Options:\n"
           "  --data <file>      data file (default: library.dat)\n"
           "  --today <date>     pretend today is <date> (YYYY-MM-DD); useful for testing fines\n"
           "  --no-save          do not write the data file (dry run)\n"
           "  --version          print version and exit\n"
           "  -h, --help         show this help\n"
           "\n"
           "The admin password is read from the LMS_ADMIN_PASSWORD environment variable\n"
           "(default: \"admin\").\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    lms::cli::Options options;
    lms::Day today = lms::currentDay();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            return 0;
        }
        if (arg == "--version") {
            std::cout << "lms " << LMS_VERSION << '\n';
            return 0;
        }
        if (arg == "--no-save") {
            options.noSave = true;
            continue;
        }
        if ((arg == "--data" || arg == "--today") && i + 1 < argc) {
            const std::string value = argv[++i];
            if (arg == "--data") {
                options.dataFile = value;
            } else if (!lms::parseDay(value, today)) {
                std::cerr << "Invalid date '" << value << "', expected YYYY-MM-DD.\n";
                return 2;
            }
            continue;
        }
        std::cerr << "Unknown or incomplete option: " << arg << "\n\n";
        printUsage(std::cerr);
        return 2;
    }

    if (const char* pw = std::getenv("LMS_ADMIN_PASSWORD"); pw && *pw) {
        options.adminPassword = pw;
    } else {
        options.adminPassword = "admin";
        std::cerr << "Warning: LMS_ADMIN_PASSWORD is not set, using the default admin password.\n";
    }

    lms::Library library;
    library.setToday(today);

    if (auto r = lms::loadFromFile(library, options.dataFile); !r) {
        std::cerr << "Cannot load " << options.dataFile << ": " << r.error << '\n';
        return 1;
    }
    if (library.books().empty() && library.students().empty()) {
        lms::seedDefaultCatalogue(library);
    }

    lms::cli::Console console(std::cin, std::cout);
    lms::cli::App app(library, console, options);
    return app.run();
}
