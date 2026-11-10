# Versioning

cminux follows a docs-frozen-between-majors policy.

## The rule

If a change would make any sentence in README.md or docs/*.md false, it is a
major version, and the docs are updated. Otherwise it is not, and the docs
are not updated.

That is the whole policy.

## Version numbers

- **Major** (X.0.0) -- the documented surface changes. Every file in
  README.md and docs/ is reviewed and updated as needed. The version line in
  each doc file is bumped. The version string in the prologue
  (src/emitter.cpp), the --version output (src/main.cpp), and CMakeLists.txt
  are bumped.
- **Minor** (1.X.0) -- capabilities are added within the existing documented
  surface. README.md and docs/ are not touched. The prologue, --version, and
  CMakeLists.txt versions are bumped.
- **Patch** (1.0.X) -- bugs are fixed. README.md and docs/ are not touched.
  The prologue, --version, and CMakeLists.txt versions are bumped.

## Documented surface

Everything a user can observe or rely on:

- The language (keywords, syntax, behavior).
- The translation (exact input and output of each rule).
- The output library (functions, overloads, behavior).
- The error messages (text, line, column, when they fire).
- The internals documented in docs/internals.md (invariants, temp file
  naming, compiler selection).
- The CLI (flags, environment variables, --version format).
- The prologue (exact text emitted at the top of generated .cpp files).
- The version line at the top of README.md and each docs/*.md file.

A change to any of these is a major version.

## Not documentation

These are code, and change on any release:

- The version string in the prologue, the --version output, and
  CMakeLists.txt. These always agree, and always reflect the current
  release, patch included.
- The keyword list in token.hpp (must match docs, changes when docs change).
- Comments in source files.
- CMakeLists.txt, .gitignore, and CI configuration.
- Test files.

## Prose typos

A typo in prose that does not describe behavior can be fixed on any release.
A mistake in a code example cannot -- code examples are part of the
documented surface.

## Old documentation

There are no docs/1.0/ or docs/current/ folders. docs/ always describes the
current major version. Old docs live in git history.

## Deprecation

cminux does not deprecate. Features are removed in a major release and
noted in the release notes. There is no "deprecated in 1.2, removed in
2.0" flow.

## Applying the rule

When you are unsure whether a change is major, ask: does this change any
sentence in README.md or docs/*.md? If yes, major. If no, not major (minor
or patch, depending on whether it adds a capability or fixes a bug).

When you are unsure whether a bug is in the code or the docs: if the docs
describe the correct behavior and the code does not match, fixing the code
is a patch. If the code is correct and the docs are wrong, fixing the docs
is a major.

## One-page summary

- Docs change only on majors.
- Majors change the documented surface, update README.md and docs/, bump
  the version in every doc file, bump the prologue / --version /
  CMakeLists.txt, and note the change in the release notes.
- Minors and patches do not touch docs. They bump the prologue / --version /
  CMakeLists.txt.
- The prologue, --version, and CMakeLists.txt always agree. They are code,
  not docs, and bump on every release.
- If a change would make a doc sentence false, it is a major. Otherwise not.