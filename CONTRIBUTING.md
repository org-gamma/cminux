# Contributing to cminux

Thanks for wanting to contribute. cminux is a small project on purpose, and
this document explains what that means for contributors: what is welcome,
what is not, how the code is organized, and what a good change looks like.

Read this whole file before opening a pull request. Most rejected changes
are rejected because they violate a rule stated here, not because the code
was bad.

## Table of contents

1. What cminux is, in one paragraph
2. What contributions are welcome
3. What contributions are not welcome
4. The design rules, restated for contributors
5. Getting the code and building it
6. Running the tests
7. Code style
8. Commit messages
9. Pull requests
10. Adding a test
11. Adding a rewrite rule
12. Adding a doc change
13. Reporting a bug
14. Asking a question
15. What happens after you open a pull request

## 1. What cminux is, in one paragraph

cminux is a source-to-source translator. It reads .cmx files, emits standard
C++17, and invokes the system C++ compiler to produce an executable. It has
nine rewrite rules. It has no parser, no AST, no type checker, no optimizer,
and no standard library beyond one header with print, println, and printf.
Everything not covered by the nine rules is C++ and passes through unchanged.
That is the whole project. See README.md for the introduction and
docs/language-reference.md for the language.

## 2. What contributions are welcome

- Bug fixes that bring the code into line with the docs. These are patches.
- Tests. More coverage is always welcome, on any release kind.
- Documentation fixes for typos in prose. Prose typos can be fixed anytime.
- Performance improvements that produce byte-identical output.
- Better error messages *for cases the docs already describe*. If the docs
  say cminux rejects X, and the current error is vague, making it clearer is
  welcome -- but only if the message text does not change from what the docs
  say. If it does, that is a major change.
- Portability fixes for compilers and platforms the code already supports
  implicitly (any C++ compiler on any OS).
- Refactorings that preserve behavior exactly.
- New tests in tests/.
- New examples in examples/.

## 3. What contributions are not welcome

These will be closed without discussion. It is not personal -- it is the
design of the project.

- A tenth rewrite rule. The language has nine rules. Adding one requires a
  major release, a plan for the next major, and a discussion about whether
  the rule fits the language. Do not open a pull request adding a rule. Open
  an issue first.
- A C++ parser. cminux does not parse C++. Adding a parser changes what
  cminux is and is out of scope for any release.
- An AST. Same reason.
- A type checker, symbol table, or name resolver. Same reason.
- Optimization passes. The C++ compiler optimizes. cminux emits.
- A standard library beyond outfn.hpp. Use C++.
- A package manager. Use CMake, vcpkg, Conan, or fetch headers manually.
- A build system. Use CMake. If you want to add an alternative, discuss it
  first.
- A language server or formatter. Those are separate tools and belong in
  tools/, not in the compiler. Discuss first.
- Deprecation warnings. cminux does not deprecate. See VERSIONING.md.
- New command-line flags. cminux has nine flags. Adding one is a major
  release. Discuss first.
- New environment variables. Same reason.
- New functions in outfn.hpp. The library is small on purpose. If you want
  a print overload for your type, put it in a header you own and import it.
  See docs/output-library.md section 7.
- Changing the emitted .cpp format. The exact output is documented. Any
  change is a major release, even if it is an improvement.
- Changing the prologue. Same reason.
- Changing error messages. Same reason.
- Changing the temp file naming or output file naming. Same reason.

If you are unsure whether your change is welcome, open an issue describing
what you want to do before writing the code. It is easier to discuss a
proposal than to reject a pull request.

## 4. The design rules, restated for contributors

Every design decision in cminux follows from four rules. If a proposed
change violates one, it is out of scope. These are not guidelines; they are
the definition of the project.

**Rule 1: cminux translates tokens, not programs.**

The rewriter looks at one token at a time, or a small fixed window around
the current token, and emits replacements. It never builds a parse tree,
never resolves names, never checks types. If a proposed change requires
knowing what a token means (rather than what it is), it is out of scope.

**Rule 2: cminux adds no semantics.**

cminux does not define what print does, what a variable is, what a function
call means. It renames keywords and moves a return type. The C++ compiler
and the C++ standard library do everything else. If a proposed change makes
cminux care about a program's meaning, it is out of scope.

**Rule 3: cminux is a strict subset of C++'s capabilities.**

Any program you can write in cminux, you can also write in C++. cminux is
not more expressive than C++; it is differently spelled. If a proposed
change makes cminux able to express something C++ cannot, it is out of
scope.

**Rule 4: the docs and the code change together, on major releases.**

VERSIONING.md defines this in detail. In practice: if you are changing what
users see, it is a major release. If you are not, you are not touching
README.md or docs/*.md.

## 5. Getting the code and building it

Clone the repository:

    git clone https://github.com/<owner>/cminux.git
    cd cminux

Build with CMake:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

On Windows, open a Developer Command Prompt first so that cl.exe is on PATH.
On macOS and Linux, clang++ or g++ is picked up automatically. If your
compiler is somewhere unusual, pass -DCMAKE_CXX_COMPILER=<path> to cmake.

The binary is at build/cminux (or build/Release/cminux.exe on Windows with
MSVC). The library header is at lib/outfn.hpp.

To build a debug version:

    cmake .. -DCMAKE_BUILD_TYPE=Debug
    cmake --build .

## 6. Running the tests

On Windows:

    tests\run.bat

On macOS and Linux, if you have bash:

    bash tests/run.sh

Set CMX_BIN if your binary is not at build/cminux (or build/cminux.exe):

    set CMX_BIN=C:\path\to\cminux.exe
    tests\run.bat

Set KEEP=1 to leave artifacts in tests/ after a run, which is useful when a
test fails and you want to see the emitted .cpp:

    set KEEP=1
    tests\run.bat

A passing run ends with "N passed, 0 failed". A failing run shows the diff,
the actual stdout vs expected stdout, or the actual exit vs expected exit,
and exits non-zero.

Before opening a pull request, make sure the full test suite passes on your
machine. A pull request that fails the tests will not be reviewed until it
passes.

## 7. Code style

cminux is C++17, built with -std=c++17, and uses no features newer than
C++17. It has no external dependencies beyond the C++ standard library.

Style is roughly:

- 4-space indentation, no tabs.
- Braces on the same line as the statement that opens them (K&R style).
- 80-column lines, soft limit (do not reflow an existing line to meet it).
- #pragma once is not used; use include guards.
- Namespaces: one per file, named cmx.
- Types: PascalCase (Token, TokKind, DriverOptions).
- Functions and variables: snake_case (tokenize, temp_cpp_path).
- Constants: UPPER_SNAKE_CASE (CMX_LIB_DIR_DEFAULT).
- Member access: this-> is not used unless needed (for disambiguation).
- nullptr, not NULL or 0.
- auto is used only when the type is obvious from the initializer or when
  the type is unwieldy.

There is no formatter. Do not introduce one. Match the surrounding code.

## 8. Commit messages

Short form:

    <area>: <imperative summary>

Examples:

    rewriter: handle empty import list
    emitter: fix spacing after pointer return types
    driver: clean up temp file on compile failure
    tests: add test for func without return type
    docs: fix typo in language-reference.md

Long form, when the change is not obvious:

    <area>: <imperative summary>

    <blank line>

    <paragraph explaining why, not what. The what is in the diff.>

    <blank line>

    <optional: "Fixes #123" or "See #456".>

Do not use conventional commits (feat:, fix:, chore:, etc.). The area prefix
already communicates the kind of change, and the changelog is not generated
from commit messages.

Do not amend a commit that has been pushed. Add a new commit instead.

## 9. Pull requests

A pull request should:

- Address one thing. Not "fix bug + add tests + refactor driver". Split it.
- Include a test that fails before the change and passes after, if the
  change is behavioral.
- Include a doc update if the change is major. VERSIONING.md defines major.
- Pass the full test suite.
- Not change the version number. Version bumps happen at release, not in
  pull requests.

Pull request description should include:

- What the change does.
- Why it is needed. If it fixes a bug, describe the bug.
- If it is a major change (per VERSIONING.md), say so explicitly and
  explain which docs change.
- If it is a minor or patch change, say so and confirm the docs are not
  affected.

Small pull requests are reviewed faster than large ones. If your change is
more than 200 lines, consider splitting it.

## 10. Adding a test

Tests live in tests/. Each test is a .cmx file, optionally accompanied by:

- <name>.expected.cpp -- the exact output of the emitter.
- <name>.expected.stdout -- the exact stdout of the compiled executable.
- <name>.expected.exit -- the exact exit code of the compiled executable.

To add a test:

1. Write tests/<name>.cmx with the input.
2. Run cminux tests/<name>.cmx --keep to produce tests/<name>.cmx.cpp.
3. Copy that file to tests/<name>.expected.cpp. Review it by hand to make
   sure it is what you expect.
4. If the program prints anything, create tests/<name>.expected.stdout with
   the exact output.
5. If the program exits non-zero, create tests/<name>.expected.exit with the
   exit code.
6. Run the test harness. All tests should pass.

Do not skip step 3. The .expected.cpp file is the contract, and if you copy
the generated file without reviewing it, you are blessing whatever the
emitter produces, including any bug it happens to contain at that moment.

Tests that are hard to write (error tests that check stderr) are currently
not supported by the harness. If you want to add one, extend the harness
first in a separate pull request.

## 11. Adding a rewrite rule

You probably should not. Adding a rewrite rule is a major release, and
requires prior discussion. If you have discussed it and want to proceed:

1. Read docs/internals.md section 10.
2. Add the keyword to is_cminux_keyword in src/token.hpp.
3. Add the case to rewrite in src/rewriter.cpp.
4. Add a test in tests/.
5. Update docs/language-reference.md: keyword list, section for the rule,
   appendix B table.
6. Update docs/translation-rules.md: new section, table of contents,
   renumbering.
7. Update docs/error-messages.md if the rule can fail.
8. Update docs/internals.md section 10, appendix B line counts, appendix C
   constants.
9. Update README.md: the rules table, the "Language (v1.0.0)" section.
10. Bump the version to X.0.0 in README.md and every docs/*.md.
11. Bump the version in src/emitter.cpp, src/main.cpp, CMakeLists.txt.

All in one pull request. The code, the tests, the docs, and the version bump
are one change, not four.

## 12. Adding a doc change

Prose typos can be fixed on any release. Fix them in a small pull request
without a version bump and without a test. Example:

    docs: fix typo "recieve" -> "receive" in error-messages.md

Substantive doc changes -- new sections, rewritten explanations, new examples
-- are major releases and require a version bump. Discuss them first.

The line in README.md and docs/*.md that says "Version: X.Y.Z" is updated
only on major releases. Do not bump it in a typo fix.

## 13. Reporting a bug

A good bug report includes:

- The version of cminux (run cminux --version).
- The operating system.
- The C++ compiler cminux is invoking (echo %CMX_CC% on Windows, echo
  $CMX_CC on Unix, or look at the output of cminux --verbose).
- The smallest .cmx file that reproduces the bug. "Smallest" matters -- if
  you can reproduce with 3 lines, do not send 300.
- The exact error message or the exact wrong output.
- What you expected instead.

If the bug is a compile error from the C++ compiler on generated code, also
include the generated .cmx.cpp file (use --keep and copy the file). That
file is the input to the C++ compiler, and seeing it usually makes the bug
obvious.

Do not report bugs that are C++ compiler bugs as cminux bugs. If the
generated .cpp file compiles under one compiler but not another, that is
almost certainly a compiler difference, not a cminux bug. Report it as a
portability issue and include the compilers you tried.

## 14. Asking a question

cminux does not have a discussion forum or a chat. If you have a question,
open an issue and label it as a question. Questions about whether a proposed
change is welcome are especially useful -- they prevent wasted work on both
sides.

If you find a real bug while asking a question, open a separate issue for
the bug. Do not mix questions and bugs in one issue.

## 15. What happens after you open a pull request

A maintainer will review your pull request, usually within a week. Reviews
are honest. If your change is out of scope, you will be told so directly,
with a link to the rule it violates. If your change is in scope but has
problems, you will be asked to fix them.

Expect the review to focus on:

- Does the change violate any design rule in section 4?
- Does it include a test?
- Does it include doc updates if the change is major?
- Does the test suite pass?
- Is the commit message in the right format?

If your pull request is closed without merge, it is not a judgment on your
code. It is a judgment on whether the change belongs in cminux. The design
rules in section 4 are the answer, and they apply equally to maintainers.

Thank you for reading this far. If you have gotten to this paragraph and
still want to contribute, you are exactly the kind of contributor cminux
needs.