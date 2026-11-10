// src/tokenizer.hpp
// Turns cminux source text into a flat token stream.
//
// Properties of the output stream (relied on by every later stage):
//   1. The last token is always TokKind::End.
//   2. Every token has a valid 1-based line and column.
//   3. A TokKind::Newline token appears at the end of every source line,
//      including the last one before End. This preserves line structure
//      through the rewriter and emitter with no extra bookkeeping.
//   4. String literals have their surrounding quotes stripped; the content
//      has \" and \\ already resolved to " and \. No other escapes are
//      processed — everything else (including \n) is left as two characters.
//      That matches how C++ will re-parse the emitted string literal, so
//      round-tripping is lossless.

#ifndef CMINUX_TOKENIZER_HPP
#define CMINUX_TOKENIZER_HPP

#include "token.hpp"
#include <string>
#include <vector>

namespace cmx {

struct TokenizeError {
    std::string message;
    int line;
    int col;
};

// Tokenize the given source. On success returns true and fills `out`.
// On failure returns false and fills `err`.
bool tokenize(const std::string& source,
              const std::string& filename,
              std::vector<Token>& out,
              TokenizeError& err);

} // namespace cmx

#endif // CMINUX_TOKENIZER_HPP