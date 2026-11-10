// lib/src/outfn.cpp
// Anchor translation unit for cminux's output library.
//
// Most of the implementation lives in outfn.hpp as `inline` functions.
// This file exists for two reasons:
//   1. It gives the library a real .obj/.o artifact, so a build system can
//      link against it explicitly rather than relying on header-only include.
//   2. It is the future home of any stateful output code (buffering, locking,
//      redirection for tests). Adding such state later should not require
//      restructuring the header.

#include "../outfn.hpp"

namespace cmx {

// Reserved for future non-inline implementation.
// Do not delete this translation unit even while empty.

} // namespace cmx