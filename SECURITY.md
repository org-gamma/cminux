# Security Policy

## What cminux is, security-wise

cminux is a developer tool. It runs on the developer's own machine, reads
.cmx files the developer wrote, emits a .cpp file, and shells out to the
C++ compiler the developer configured. It is not a server, not a sandbox,
and not a service that processes untrusted input.

This matters for reporting: the class of vulnerabilities that apply to
cminux is narrow, and most "vulnerabilities" reported against developer
tools of this kind are not vulnerabilities at all. The sections below
explain what counts, what does not, and where to report the real ones.

## Supported Versions

Only the current major version receives security updates. Older majors are
frozen: they do not receive fixes of any kind, including security fixes.

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

Because cminux follows a docs-frozen-between-majors policy (see
VERSIONING.md), security fixes land as patch releases within the current
major. A security fix will not be backported to an older major.

## What counts as a vulnerability

cminux has three components that can plausibly have security issues.

### 1. The compiler driver (src/compiler.cpp)

cminux builds a command string and calls `std::system` to invoke the C++
compiler. If a user-supplied path (the input file, the output file, an
include directory, or an extra compiler flag) can inject arbitrary commands
into that string, an attacker who controls the path can run arbitrary code
on the developer's machine.

This is a real vulnerability *if* the path comes from somewhere the
developer does not control. In normal use, the developer types the path
themselves. If you are building a system that invokes cminux on
attacker-controlled filenames, and the filenames contain shell
metacharacters that escape `shell_quote`, that is a cminux vulnerability.

Report it. Include the exact path that escapes quoting, and the platform
(Windows vs. POSIX) where it happens, because the quoting rules differ.

### 2. The output library (lib/outfn.hpp)

`print` and `println` forward to `std::cout`. `printf` forwards to
`std::vprintf`. A format-string vulnerability in `printf` is a C++
vulnerability, not a cminux one — but if cminux's wrapper introduces a new
issue (for example, if the wrapper passes the wrong argument types to
`vprintf`), that is cminux's bug.

Report it. Include the cminux source that triggers the issue and the
generated .cpp file (from `--keep`).

### 3. The transpiler itself (src/tokenizer.cpp, src/rewriter.cpp, src/emitter.cpp, src/driver.cpp)

The transpiler reads a `.cmx` file and writes a `.cpp` file. The bugs that
matter are those that allow a crafted `.cmx` file to:

- Write a file outside the intended path (path traversal via the temp file
  name).
- Read a file the user did not ask to read (there is no such path in
  v1.0.0 — the tokenizer reads only the input file).
- Cause a buffer overflow, use-after-free, or undefined behavior in the
  transpiler process itself.
- Cause the emitted .cpp file to differ from what the docs say it emits, in
  a way that produces malicious code. (This is unlikely — the emitted file
  is whatever the input tokens said, modulo the nine rules — but if you
  find a way to make the emitter inject code that was not in the input,
  that is a vulnerability.)

Report it. Include the smallest `.cmx` file that reproduces the issue, and
the actual vs. expected output.

## What does not count as a vulnerability

These will be closed as "not a security issue" without further discussion.
The list is here so you do not waste time on a report.

- **cminux compiles and runs malicious C++.** If your .cmx file contains a
  `system("rm -rf /")` call, cminux will faithfully translate it and the
  resulting program will run `rm -rf /`. This is not a cminux vulnerability.
  cminux does not sandbox the programs it compiles, does not analyze what
  they do, and is not expected to. Running arbitrary C++ is the entire point
  of a C++ compiler.

- **cminux shells out to the C++ compiler.** Yes, it does. That is the
  design. If you do not want cminux to invoke your C++ compiler, do not run
  cminux. See src/compiler.cpp.

- **cminux creates a temp file next to the input.** It does. The temp file
  is named `<stem>.cmx.cpp` and is removed after compilation unless --keep
  is passed. If your threat model includes "an attacker can write to the
  directory where the input lives," you have already lost — the attacker can
  write to your input file too.

- **cminux trusts the C++ compiler.** It does. If your C++ compiler is
  compromised, everything cminux does is compromised. That is a property of
  your system, not of cminux.

- **A malformed .cmx file crashes cminux.** A crash in the transpiler on
  malformed input is a robustness bug, not a security bug, *unless* the
  crash is exploitable (memory corruption that can be turned into code
  execution). Crash-only bugs are reported as regular bugs, not security
  issues. See CONTRIBUTING.md section 13.

- **Compile-time denial of service.** A .cmx file that takes a long time to
  tokenize (for example, a gigabyte of input) is not a DoS vulnerability.
  cminux runs on the developer's machine and the developer chose to compile
  the file. DoS in a developer tool is a performance bug.

- **The output library's flush-on-every-call behavior slows programs
  down.** See the previous point.

- **cminux does not validate C++ output.** It does not. The C++ compiler
  does. That is the design.

If your report falls into one of these categories, it will be closed with a
link to this section. That is not a judgment of your work — it is a
judgment of whether the issue matches the threat model.

## Reporting a Vulnerability

Do not open a public issue for a security vulnerability. Public issues are
for bugs that anyone can see without risk; security issues are not.

Report privately by email to:

    security@gamma.org

If the project does not have a dedicated security email, use the maintainer
email listed in the repository's commit history. If you cannot find an
email, open a GitHub issue titled "Security contact needed" and a maintainer
will reach out privately — do not include details of the vulnerability in
that issue.

Include in your report:

- A description of the vulnerability.
- The version of cminux (run cminux --version).
- The operating system and C++ compiler.
- The smallest .cmx file (or command-line invocation) that reproduces the
  issue.
- The impact: what an attacker can do, and under what conditions.
- Any suggested fix or mitigation you have in mind.

If you have a working proof of concept, include it. If you do not, a clear
description is enough.

## What to expect after you report

You will receive an acknowledgement within 7 days. If you do not, send a
follow-up email — sometimes reports are caught by spam filters, and a
follow-up is the fastest way to get a human response.

After the acknowledgement:

- **If the report is accepted:** the maintainer will work on a fix, keep
  you informed of progress, and credit you in the release notes when the
  fix lands, unless you ask to remain anonymous. The fix will be a patch
  release within the current major (for example, 1.0.1, 1.0.2), because
  security fixes are not a change to the documented surface and do not
  require a major version bump.

- **If the report is declined:** the maintainer will explain why. The
  explanation will reference a section of this document, usually "What does
  not count as a vulnerability." If you disagree with the reasoning, reply
  and explain — the maintainer will reconsider if you make a case for why
  the issue fits the threat model.

- **If the report is a bug but not a security issue:** the maintainer will
  ask you to open a public issue, or will open one on your behalf. Regular
  bugs are handled in the public issue tracker.

## Disclosure timeline

The maintainer's goal is:

- 7 days: acknowledgement.
- 30 days: a fix, a mitigation, or a decision to decline.
- 90 days: public disclosure, coordinated with you.

If a fix takes longer than 90 days, the maintainer will explain why. If you
want to disclose earlier than 90 days because the issue is actively being
exploited, say so in your report — the timeline is a goal, not a contract,
and active exploitation changes the calculus.

cminux does not pay bug bounties. It does not have a program, a budget, or
a process for them. Reports are appreciated and credited, not compensated.

## Scope

In scope:

- The cminux binary and its source code (src/).
- The output library (lib/outfn.hpp, lib/src/outfn.cpp).
- The build scripts (CMakeLists.txt, tests/run.bat, tests/run.sh).

Out of scope:

- The C++ compiler cminux invokes. Report those to the compiler's
  maintainers (MSVC, GCC, Clang).
- Third-party libraries cminux does not depend on (it has none, but if you
  add one, it is out of scope).
- The developer's machine, operating system, or shell.
- The `.cmx` files cminux compiles. Those are the user's code, not cminux's.
- The `.cpp` files cminux emits. Those are the result of compiling the
  user's code, and any vulnerability in them is a vulnerability in the
  user's code, not in cminux.

## A note on the honest scope of this document

cminux has a small attack surface. It reads a file, rewrites tokens, writes
a file, and runs a command. The realistic security issues are:

1. Command injection via a path that escapes shell_quote.
2. A memory safety bug in the tokenizer, rewriter, emitter, or driver that
   is exploitable.
3. A bug in the output library that causes the emitted program to behave
   differently from what the developer intended.

If you find something outside these three categories, it may still be worth
reporting, but it is likely to be classified as a bug rather than a
vulnerability, and the response will reflect that.

The short version: report real issues, skip the ones this document says are
out of scope, and if you are unsure, ask first by opening a non-detail
issue or emailing the maintainer. A five-minute conversation beats a
rejected report.

Thank you for helping keep cminux and its users safe.