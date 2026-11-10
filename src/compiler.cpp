// src/compiler.cpp
// Shell-out to the host C++ compiler.
//
// Uses std::system for portability. This is not the most secure way to invoke
// a subprocess, but it is correct for a developer tool that operates on the
// user's own files. Do not use this pattern in a server that compiles
// untrusted input.

#include "compiler.hpp"
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>

namespace cmx {

namespace {

// Quote a path for the platform shell.
std::string shell_quote(const std::string& s) {
#if defined(_WIN32)
    // Windows cmd.exe: wrap in double quotes, double any embedded quotes.
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    // POSIX: single-quote, escape embedded single quotes as '\''
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

} // namespace

std::string detect_compiler() {
    if (const char* env = std::getenv("CMX_CC")) {
        if (env[0] != '\0') return env;
    }
#if defined(_WIN32)
    return "cl";
#elif defined(__APPLE__)
    return "clang++";
#else
    return "g++";
#endif
}

bool compile_to_exe(const CompileOptions& opts, std::string& err) {
    std::string cc = opts.compiler.empty() ? detect_compiler() : opts.compiler;

    std::ostringstream cmd;
    cmd << cc;
    cmd << ' ' << shell_quote(opts.cpp_source);

    for (const auto& inc : opts.include_dirs) {
        cmd << " -I" << shell_quote(inc);
    }
    for (const auto& flag : opts.extra_flags) {
        // Extra flags are passed verbatim; the user owns their quoting.
        cmd << ' ' << flag;
    }

#if defined(_WIN32)
    if (cc == "cl" || cc == "cl.exe") {
        cmd << " /Fe:" << shell_quote(opts.output_exe);
    } else {
        cmd << " -o " << shell_quote(opts.output_exe);
    }
#else
    cmd << " -o " << shell_quote(opts.output_exe);
#endif
    cmd << " -std=c++17";

    if (opts.verbose) {
        std::cerr << "cminux: " << cmd.str() << "\n";
    }

    int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        std::ostringstream m;
        m << "compiler exited with status " << rc;
        err = m.str();
        return false;
    }
    return true;
}

} // namespace cmx