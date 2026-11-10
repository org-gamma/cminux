// src/compiler.hpp
// Invokes the host C++ compiler to turn a generated .cpp into an executable.

#ifndef CMINUX_COMPILER_HPP
#define CMINUX_COMPILER_HPP

#include <string>
#include <vector>

namespace cmx {

struct CompileOptions {
    std::string cpp_source;       // path to generated .cpp
    std::string output_exe;       // path to output executable
    std::vector<std::string> include_dirs;  // -I
    std::vector<std::string> extra_flags;   // passed verbatim
    std::string compiler;         // empty = auto-detect
    bool verbose = false;
};

// Returns true on success. On failure, `err` is filled with a human-readable
// message and the compiler's own stderr is forwarded to our stderr.
bool compile_to_exe(const CompileOptions& opts, std::string& err);

// Returns the compiler to use, consulting CMX_CC env var first, then
// a platform default, then falling back to `c++` on PATH.
std::string detect_compiler();

} // namespace cmx

#endif // CMINUX_COMPILER_HPP