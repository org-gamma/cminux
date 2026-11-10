// src/rewriter.hpp
// Transforms a cminux token stream into an equivalent C++ token stream.
//
// Every rule in this pass is LOCAL and TYPE-AGNOSTIC. It looks at the current
// token, sometimes the next one, and emits zero or more replacement tokens.
// It never looks at more than a small fixed window. This property is what
// keeps cminux from becoming a real compiler.
//
// Rules in v0.1:
//   import [ "a.hpp", "b.hpp" ]   ->  #include "a.hpp"\n#include "b.hpp"
//   func                          ->  (dropped)
//   render                        ->  return
//   nil                           ->  nullptr
//   let                           ->  const auto
//   var                           ->  auto
//
// `print` and `println` are NOT rewritten. They resolve through using
// declarations in outfn.hpp. If you want to add a rule, add it here; do
// not add a new pass.

#ifndef CMINUX_REWRITER_HPP
#define CMINUX_REWRITER_HPP

#include "token.hpp"
#include <vector>
#include <string>

namespace cmx {

struct RewriteError {
    std::string message;
    int line;
    int col;
};

// Rewrites `in` into `out`. Returns false and fills `err` on error.
// The output always ends with a TokKind::End token.
bool rewrite(const std::vector<Token>& in,
             std::vector<Token>& out,
             RewriteError& err);

} // namespace cmx