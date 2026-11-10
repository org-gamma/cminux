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
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

bool compiler_available(const std::string& name) {
#if defined(_WIN32)
    std::string cmd = "where " + name + " >nul 2>nul";
#else
    std::string cmd = "command -v " + name + " >/dev/null 2>&1";
#endif
    return std::system(cmd.c_str()) == 0;
}

} // namespace

std::string detect_compiler() {
    if (const char* env = std::getenv("CMX_CC")) {
        if (env[0] != '\0') return env;
    }

#if defined(_WIN32)
    if (compiler_available("g++"))     return "g++";
    if (compiler_available("clang++")) return "clang++";
    if (compiler_available("cl"))      return "cl";
    return "cl";
#elif defined(__APPLE__)
    if (compiler_available("clang++")) return "clang++";
    if (compiler_available("g++"))     return "g++";
    return "clang++";
#else
    if (compiler_available("g++"))     return "g++";
    if (compiler_available("clang++")) return "clang++";
    return "g++";
#endif
}

bool compile_to_exe(const CompileOptions& opts, std::string& err) {
    std::string cc = opts.compiler.empty() ? detect_compiler() : opts.compiler;

    bool is_msvc = (cc == "cl" || cc == "cl.exe");

    std::ostringstream cmd;

    // The compiler name is NOT quoted. It is a bare command name resolved
    // via PATH. Quoting it on Windows makes cmd.exe interpret the quotes
    // literally, which produces the error:
    //   'g++" "hello.cmx.cpp" -I"D:' is not recognized
    cmd << cc;

    // Paths ARE quoted. They may contain spaces (the project directory
    // here is "D:\Projects WIP\cminux", which has a space).
    cmd << ' ' << shell_quote(opts.cpp_source);

    for (const auto& inc : opts.include_dirs) {
        if (is_msvc) {
            cmd << " /I" << shell_quote(inc);
        } else {
            cmd << " -I" << shell_quote(inc);
        }
    }
    for (const auto& flag : opts.extra_flags) {
        cmd << ' ' << flag;
    }

    if (is_msvc) {
        cmd << " /Fe:" << shell_quote(opts.output_exe);
        cmd << " /std:c++17";
    } else {
        cmd << " -o " << shell_quote(opts.output_exe);
        cmd << " -std=c++17";
    }

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