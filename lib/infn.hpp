// lib/infn.hpp
// cminux input library — read_int, read_double, read_word, read_line, read_all.

#ifndef CMINUX_INFN_HPP
#define CMINUX_INFN_HPP

#include <iostream>
#include <string>
#include <limits>

namespace cmx {

inline int read_int() {
    while (true) {
        int x;
        if (std::cin >> x) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return x;
        }
        if (std::cin.eof()) return 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

inline int read_int(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::cout.flush();
        int x;
        if (std::cin >> x) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return x;
        }
        if (std::cin.eof()) return 0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "not a number, try again\n";
    }
}

inline double read_double() {
    while (true) {
        double x;
        if (std::cin >> x) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return x;
        }
        if (std::cin.eof()) return 0.0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

inline double read_double(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::cout.flush();
        double x;
        if (std::cin >> x) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return x;
        }
        if (std::cin.eof()) return 0.0;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "not a number, try again\n";
    }
}

inline std::string read_word() {
    std::string s;
    std::cin >> s;
    return s;
}

inline std::string read_line() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

inline std::string read_all() {
    std::string s;
    std::string line;
    while (std::getline(std::cin, line)) {
        s += line;
        s += '\n';
    }
    return s;
}

} // namespace cmx

using cmx::read_int;
using cmx::read_double;
using cmx::read_word;
using cmx::read_line;
using cmx::read_all;

#endif // CMINUX_INFN_HPP