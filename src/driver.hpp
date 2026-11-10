// src/driver.hpp
// Orchestrates the full cminux pipeline for one source file.

#ifndef CMINUX_DRIVER_HPP
#define CMINUX_DRIVER_HPP

#include <string>
#include <vector>

namespace cmx {

struct DriverOptions {
    std::string input_path;                 // required
    std::string output_exe;                 // optional; derived if empty
    std::vector<std::string> include_dirs;  // -I
    std::vector<std::string> extra_flags;   // passed to the C++ compiler
    std::string compiler;                   // empty = auto-detect
    bool keep_cpp = false;                  // keep the generated .cpp
    bool verbose = false;                   // print the pipeline commands
    bool dump_tokens = false;               // print tokens to stderr
};

// Returns process exit code: 0 on success, non-zero on failure.
int run(const DriverOptions& opts);

} // namespace cmx

#endif // CMINUX_DRIVER_HPP