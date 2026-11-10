// src/emitter.hpp
// Prints a rewritten token stream as a complete C++ translation unit.
//
// The emitter is a dumb token printer. It has three jobs and no others:
//   1. Print the prologue: a marker comment, the #include of outfn.hpp,
//      and a `using` directive so cminux's print/println resolve.
//   2. Print the rewritten tokens with normal C++ spacing.
//   3. Preserve line structure so C++ compiler diagnostics map back to
//      .cmx lines (approximately).
//
// If you find yourself wanting the emitter to understand a construct,
// stop. That knowledge belongs in the rewriter.

#ifndef CMINUX_EMITTER_HPP
#define CMINUX_EMITTER_HPP

#include "token.hpp"
#include <string>
#include <vector>

namespace cmx {

struct EmitOptions {
    std::string source_filename;  // used in the prologue comment and #line
    bool        emit_line_directive = true;
    bool        emit_prologue = true;
};

std::string emit_cpp(const std::vector<Token>& tokens,
                     const EmitOptions& opts);

} // namespace cmx