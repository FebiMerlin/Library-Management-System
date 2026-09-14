#include "lms/storage.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace lms {

namespace {

constexpr const char* kHeader = "LMS\t1";

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : line) {
        if (c == '\t') {
            parts.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

bool parseInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    std::size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size()) return false;
    long long v = 0;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
        if (v > 2147483647LL) return false;
    }
    out = static_cast<int>(s[0] == '-' ? -v : v);
    return true;
}

bool parseInt64(const std::string& s, std::int64_t& out) {
    if (s.empty()) return false;
    std::size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size() || s.size() > 19) return false;
    std::int64_t v = 0;
    for (; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    out = s[0] == '-' ? -v : v;
    return true;
}

Result formatError(std::size_t lineNo, const std::string& what) {
    return Result::failure("Data file, line " + std::to_string(lineNo) + ": " + what);
}

}  // namespace

std::string serialize(const Library& library) {
    std::ostringstream out;
    out << kHeader << '\n';
    for (const Book& b : library.books()) {
        out << "BOOK\t" << b.isbn << '\t' << b.title << '\t' << b.author << '\t' << b.issuedTo << '\t' << b.issuedOn
            << '\n';
    }
    for (const Student& s : library.students()) {
        out << "STUDENT\t" << s.roll << '\t' << s.name << '\t' << s.balance << '\n';
    }
    for (const Transaction& t : library.transactions()) {
        out << "TX\t" << t.roll << '\t' << t.day << '\t' << toString(t.type) << '\t' << t.amount << '\t'
            << t.balanceAfter << '\t' << t.isbn << '\n';
    }
    return out.str();
}

Result deserialize(const std::string& text, Library& library) {
    std::istringstream in(text);
    std::string line;
    std::size_t lineNo = 0;

    if (!std::getline(in, line)) return Result::failure("Data file is empty.");
    ++lineNo;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != kHeader) return formatError(lineNo, "unknown format or version (expected 'LMS 1').");

    std::vector<Book> books;
    std::vector<Student> students;
    std::vector<Transaction> txs;

    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty() || line == "\r") continue;
        const auto f = splitTabs(line);
        const std::string& kind = f[0];

        if (kind == "BOOK") {
            if (f.size() != 6) return formatError(lineNo, "BOOK record must have 5 fields.");
            Book b;
            if (!parseInt(f[1], b.isbn) || !parseInt(f[4], b.issuedTo) || !parseInt(f[5], b.issuedOn)) {
                return formatError(lineNo, "invalid number in BOOK record.");
            }
            b.title = f[2];
            b.author = f[3];
            if (auto r = Library::validateIsbn(b.isbn); !r) return formatError(lineNo, r.error);
            for (const Book& existing : books) {
                if (existing.isbn == b.isbn) return formatError(lineNo, "duplicate ISBN.");
            }
            books.push_back(b);
        } else if (kind == "STUDENT") {
            if (f.size() != 4) return formatError(lineNo, "STUDENT record must have 3 fields.");
            Student s;
            if (!parseInt(f[1], s.roll) || !parseInt64(f[3], s.balance)) {
                return formatError(lineNo, "invalid number in STUDENT record.");
            }
            s.name = f[2];
            if (auto r = Library::validateRoll(s.roll); !r) return formatError(lineNo, r.error);
            for (const Student& existing : students) {
                if (existing.roll == s.roll) return formatError(lineNo, "duplicate roll number.");
            }
            students.push_back(s);
        } else if (kind == "TX") {
            if (f.size() != 7) return formatError(lineNo, "TX record must have 6 fields.");
            Transaction t;
            if (!parseInt(f[1], t.roll) || !parseInt(f[2], t.day) || !parseTxType(f[3], t.type) ||
                !parseInt64(f[4], t.amount) || !parseInt64(f[5], t.balanceAfter) || !parseInt(f[6], t.isbn)) {
                return formatError(lineNo, "invalid field in TX record.");
            }
            txs.push_back(t);
        } else {
            return formatError(lineNo, "unknown record type '" + kind + "'.");
        }
    }

    // Referential integrity: an issued book must belong to a known student.
    for (const Book& b : books) {
        if (b.issuedTo == 0) continue;
        bool found = false;
        for (const Student& s : students) found = found || s.roll == b.issuedTo;
        if (!found) {
            return Result::failure("Data file: book " + std::to_string(b.isbn) + " is issued to unknown student " +
                                   std::to_string(b.issuedTo) + ".");
        }
    }

    library.restore(std::move(books), std::move(students), std::move(txs));
    return Result::success();
}

Result saveToFile(const Library& library, const std::string& path) {
    // Write to a temporary file first so a crash mid-write cannot corrupt the
    // existing data file.
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return Result::failure("Cannot open '" + tmp + "' for writing.");
        out << serialize(library);
        if (!out) return Result::failure("Failed while writing '" + tmp + "'.");
    }
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        return Result::failure("Cannot replace '" + path + "' with the new data file.");
    }
    return Result::success();
}

Result loadFromFile(Library& library, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    // is_open() rather than operator!: some toolchains do not set failbit
    // when the constructor fails to open the file.
    if (!in.is_open()) return Result::success();  // first run: nothing to load
    std::stringstream buffer;
    buffer << in.rdbuf();
    return deserialize(buffer.str(), library);
}

void seedDefaultCatalogue(Library& library) {
    struct Entry {
        int isbn;
        const char* title;
        const char* author;
    };
    static const Entry kCatalogue[] = {
        {1001, "The C++ Programming Language", "Bjarne Stroustrup"},
        {1002, "Effective Modern C++", "Scott Meyers"},
        {1003, "Clean Code", "Robert C. Martin"},
        {1004, "The Pragmatic Programmer", "Andrew Hunt, David Thomas"},
        {1005, "Design Patterns", "Erich Gamma et al."},
        {1006, "Introduction to Algorithms", "Thomas H. Cormen et al."},
        {1007, "Structure and Interpretation of Computer Programs", "Harold Abelson, Gerald Jay Sussman"},
        {1008, "Code Complete", "Steve McConnell"},
        {1009, "Refactoring", "Martin Fowler"},
        {1010, "The Mythical Man-Month", "Frederick P. Brooks Jr."},
        {1011, "Programming: Principles and Practice Using C++", "Bjarne Stroustrup"},
        {1012, "C++ Concurrency in Action", "Anthony Williams"},
        {1013, "Working Effectively with Legacy Code", "Michael Feathers"},
        {1014, "Continuous Delivery", "Jez Humble, David Farley"},
        {1015, "The Art of Computer Programming, Vol. 1", "Donald E. Knuth"},
    };
    for (const Entry& e : kCatalogue) library.addBook(e.isbn, e.title, e.author);
}

}  // namespace lms
