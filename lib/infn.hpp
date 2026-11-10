// lib/infn.hpp
// cminux input library — read_int, read_double, read_word, read_line, read_all.
//
// All functions read from std::cin. None of them opens files, prompts, or
// prints anything except the retry prompt in read_int and read_double
// (which is opt-in; see below). Everything is a thin wrapper over
// std::cin and std::getline, with the one piece of real logic being
// the retry-on-failure loop in the numeric readers.

#ifndef CMINUX_INFN_HPP
#define CMINUX_INFN_HPP

#include <iostream>
#include <string>
#include <limits>

namespace cmx {

// ---------------------------------------------------------------------------
// read_int
//
//   read_int()            -> int, retrying on bad input
//   read_int(prompt)      -> same, but prints prompt first
//
// Reads one integer from std::cin. If the input is not a valid integer,
// clears the stream's error state, discards the rest of the line, prints
// "not a number, try again: " (or the caller's prompt) and reads again.
// This loops until a valid integer is read or stdin reaches EOF, at
// which point it returns 0 and the stream is left in a failed state.
//
// The no-argument form does not print anything on bad input; it silently
// retries. This is the right default for programs that are fed input
// from a pipe or a file, where "not a number, try again" makes no sense.
// Programs that read from a human should use the prompt form.
// ---------------------------------------------------------------------------

inline int read_int() {
    while (true) {
        int x;
        if (std::cin >> x) {
            // Consume the rest of the line so that a subsequent
            // read_line() does not see an empty line. This is the
            // behavior most programs want; without it, mixing
            // read_int and read_line produces surprising skips.
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

// ---------------------------------------------------------------------------
// read_double
//
//   read_double()         -> double, retrying on bad input
//   read_double(prompt)   -> same, with prompt
//
// Identical to read_int but for double. Accepts integer-form input
// ("42" reads as 42.0), decimal ("3.14"), and scientific ("1e10").
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// read_word
//
//   read_word() -> one whitespace-delimited token, as std::string
//
// Reads one word: the first non-whitespace run of characters, stopping
// at the next whitespace. Leading whitespace is skipped. If stdin
// reaches EOF before a word is read, returns "".
//
// Does not consume the rest of the line. A caller who wants to read
// word by word from a line should call read_word() repeatedly until it
// returns "" or until read_line() returns; mixing them is the caller's
// responsibility.
// ---------------------------------------------------------------------------

inline std::string read_word() {
    std::string s;
    std::cin >> s;
    return s;
}

// ---------------------------------------------------------------------------
// read_line
//
//   read_line() -> one full line, as std::string, with the newline removed
//
// Reads until the next '\n' and returns everything before it. The
// newline itself is consumed but not included in the result. If stdin
// is at EOF, returns "".
//
// This is the standard std::getline(std::cin, s) wrapper. It exists
// because cminux source should not have to write std::getline to read
// a line, and because the naming (read_line, not getline) is clearer
// in a language that also has read_int and read_word.
// ---------------------------------------------------------------------------

inline std::string read_line() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// ---------------------------------------------------------------------------
// read_all
//
//   read_all() -> everything on stdin until EOF, as std::string
//
// Reads the entire remaining input in one call. Useful for programs
// that process their input as a whole (a filter, a cat, a line
// counter). Returns "" if stdin is already at EOF.
//
// This is the only function in the library that reads past a newline.
// All the others stop at a line or a token boundary. If you call
// read_all() and then read_line(), the second call returns "" because
// stdin is exhausted.
// ---------------------------------------------------------------------------

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

#endif // CMINUX_INFN_HPP