# cminux

A simpler C++ that compiles to native executables.

cminux is a small, opinionated language that looks like a cleaned-up C++ and
compiles to it. You write .cmx files. cminux translates them to standard
C++17, invokes your system C++ compiler, and hands you a native executable.
There is no interpreter, no virtual machine, no garbage collector, and no
runtime beyond a single header that gives you print and println.

If you have ever written C++ and wished the boilerplate would get out of the
way -- wished that main could just be main, that return could be a word you
actually remember, that declaring a variable did not require choosing
between auto, decltype, const auto, and three forms of brace initialization
-- cminux is for you. It keeps C++'s speed, its control, and its ecosystem.
It removes the ceremony.


## What cminux is

cminux is a transpiler. It reads a .cmx source file and writes a .cpp file.
That .cpp file is ordinary, portable C++17. Nothing about it is special,
hidden, or runtime-dependent. You could take the generated file, copy it
into any C++ project, and it would compile and run exactly as it does under
cminux.

Then cminux invokes the C++ compiler on your machine -- cl.exe on Windows
with MSVC, g++ or clang++ elsewhere -- and produces an executable. From your
point of view, one command:

    cminux hello.cmx -o hello.exe

produces hello.exe. From the compiler's point of view, it compiled a
perfectly normal C++ program. Both are true, and that is the whole design.

cminux is not a new compiler. It does not parse C++, does not type-check,
does not optimize, does not do code generation. It does one job: replace a
small set of keywords and rewrite one construct (func ... -> T), then hand
the result to the real C++ compiler. Everything else -- templates, classes,
inheritance, the standard library, operator overloading, lambdas, raw
pointers, inline assembly, everything -- is C++, unchanged, and works
because it is compiled by C++.

The consequence is important: any C++ you can write, cminux can compile,
because cminux does not touch it. The flip side is also true: cminux does
not make C++ safer, does not add a borrow checker, does not prevent you from
shooting yourself in the foot. It removes typing, not responsibility.


## What cminux is not

It is worth being clear about this, because "simpler C++" can mean many
things and cminux only means one of them.

cminux is not a C++ replacement. It does not try to be a better language in
the way that Rust or Zig or Carbon are. It is not a new memory model, not a
new type system, not a new concurrency story. It is C++ with a shorter
vocabulary. If you want a fundamentally different C++, cminux is the wrong
tool and will disappoint you.

cminux is not a safer C++. Pointers are pointers. Buffer overruns are
buffer overruns. Undefined behavior is undefined behavior. You get the same
compiler, the same flags, the same warnings, and the same consequences.
cminux makes the source shorter; it does not make the program more correct.

cminux is not a build system. It is a single-file compiler front end. If you
have ten .cmx files, cminux compiles each one; you still need to link them,
and for that you should keep using CMake, Make, or whatever you already use.
cminux integrates with your existing build by compiling one file at a time,
the same way a C++ compiler does.

cminux is not a superset of C++. A .cmx file is not a valid .cpp file,
because func and render are not C++ keywords and import [...] is not C++
syntax. cminux code cannot be compiled directly by g++ or cl.exe without
going through cminux. This is a deliberate trade: cminux gives up "the
source compiles as-is" in exchange for a syntax that reads the way you want
to read it. The output compiles as-is, always.

cminux is not trying to be your only language. It is designed to be used for
small programs, teaching, prototypes, examples, and scripts where the
ceremony of C++ gets in the way. If you have a 200,000-line codebase, keep
using C++ directly. cminux is for the 200-line programs that should have
taken 20 lines.


## Why cminux exists

C++ is one of the most powerful languages ever made. It is also one of the
most exhausting to read. Not because the ideas are hard -- they are not --
but because the syntax carries four decades of backward compatibility, and
every new feature has to fit alongside every old one.

Consider a few lines of ordinary C++:

    auto x = 5;
    const auto y = compute();
    auto& ref = *ptr;
    decltype(auto) z = forward(x);

Four different keywords to say "a variable whose type is inferred." Four
different semantics, four different mental models, four different things to
remember. For someone learning the language, this is noise. For someone who
has used C++ for twenty years, it is habit -- but it is still noise.

cminux collapses them:

    var x = 5;
    let y = compute();
    var r = *ptr;         // references are still C++ references
    var z = forward(x);   // forwarding is still C++ forwarding

var means "a variable I will change." let means "a variable I will not
change." That is the entire distinction, and it is the one that matters 99%
of the time. If you need decltype(auto) specifically, C++ is one --keep flag
away and you can write the exact type you want.

Consider returning a value:

    int main() {
        return 0;
    }

Familiar if you have used C++, opaque if you have not. Why return? Why int
before main? Why the braces? Every C-family language has this exact shape,
and every learner hits it on day one. cminux says:

    func main() -> int {
        render 0;
    }

The function is a func. It returns an int, and the arrow points to it. It
returns this value with render. The shape is the same; the words are the
ones you would use if you were explaining it out loud. If you have used
Swift, Rust, Go, Kotlin, TypeScript, or half a dozen other modern languages,
this looks familiar for a reason: they all converged on this shape because
it reads well.

Consider bringing in a library:

    #include "outfn.hpp"
    #include "math.hpp"

Two lines, one prefix, one convention. It works, and it is fine. cminux
says:

    import ["outfn.hpp", "math.hpp"]

One line, one keyword, one list. The expansion to two #includes happens
during translation. If you have used Python, JavaScript, Rust, Go, Java, or
literally any language with an import or use keyword, this is what you
expect.

None of these changes are deep. None of them affect what the program does,
how fast it runs, what it can express. They are all surface, and yet
together they change how the language feels to read. That is the bet cminux
makes: that the surface matters, that a language with a cleaner surface is a
language you want to spend time in, and that you do not need to sacrifice
C++'s power to get it.


## How it works

The pipeline is short and you should understand all of it. There is no
magic, and once you have read this section, nothing about cminux will
surprise you.

    hello.cmx
       |
       v
    tokenizer      -> a flat list of tokens (keywords, identifiers, strings,
                      numbers, punctuation)
       |
       v
    rewriter       -> the same list, with a handful of tokens replaced:
                      import [...] -> #include lines
                      func         -> (dropped; the return type moves)
                      ->           -> (dropped; the return type moves)
                      render       -> return
                      nil          -> nullptr
                      let          -> const auto
                      var          -> auto
       |
       v
    emitter        -> a complete .cpp file, with a prologue and #line directives
       |
       v
    hello.cmx.cpp
       |
       v
    C++ compiler   -> hello.exe

That is the whole thing. The tokenizer is a single pass over the source. The
rewriter is a single pass over the tokens. The emitter is a single pass over
the rewritten tokens. The compiler is your system C++ compiler, invoked as a
subprocess.

There is no abstract syntax tree. There is no type checker. There is no
symbol table, no name resolution, no optimization pass, no intermediate
representation. The rewriter does not know what int means, does not know
that main is special, does not know that print is a function. It knows that
render is four letters that should become six, and that func should
disappear, and it does those things without understanding them.

This is deliberate. The moment the rewriter needs to know what a type is,
cminux has become a compiler, and building a compiler is a five-year project.
The moment the rewriter needs to know what print does, cminux has become a
language with a standard library, and building a standard library is a
ten-year project. cminux does neither. It translates words, and the C++
compiler and the C++ standard library do everything else.

The one construct that requires more than a local substitution is
func ... -> T. To turn:

    func add(a: int, b: int) -> int {

into:

    int add(a: int, b: int) {

the rewriter has to find the ->, copy the type that follows it, and move it
to before the function name. That requires scanning the parameter list with
balanced parentheses to find where the parameters end, and then scanning
forward from -> to find where the return type ends. It is still
type-agnostic -- the rewriter copies type tokens without interpreting them
-- but it is not a single-token lookahead. It is the one place where cminux
looks more than a few tokens ahead, and it is worth knowing it exists.

Everything else is local. render becomes return. nil becomes nullptr. let
becomes const auto. var becomes auto. The word is replaced, nothing else
changes.


## What the translation looks like

The best way to understand cminux is to see it. Here is a complete program.

Source, hello.cmx:

    import ["outfn.hpp"]
    func main() -> int {
        print("There is no newline at the end! ");
        println("There's a newline!");
        render 0;
    }

Output, hello.cmx.cpp:

    // generated by cminux v1.0.0 -- do not edit
    // source: hello.cmx
    #include "outfn.hpp"
    using cmx::print;
    using cmx::println;

    #line 1 "hello.cmx"
    #include "outfn.hpp"
    int main() {
        print("There is no newline at the end! ");
        println("There's a newline!");
        return 0;
    }

Run it, and you see:

    There is no newline at the end! There's a newline!

Three things are worth noticing.

First, the generated file has a prologue. Four lines are prepended: a
comment marking it as generated, a comment naming the source, the
#include "outfn.hpp", and two using declarations that bring print and
println into scope. The prologue is what makes print("hello") work in cminux
source without requiring cmx::print("hello"). It is emitted once, at the
top, always, and you never have to think about it.

Second, the generated file has a #line directive:

    #line 1 "hello.cmx"

This tells the C++ compiler "the next line I give you is line 1 of
hello.cmx." From that point on, if the C++ compiler reports an error on line
7 of the generated file, it will report it as an error on line 7 of
hello.cmx -- the file you wrote, with the line numbers you see in your
editor. The prologue and the #line are how cminux hides its own output from
your error messages. Without them, every compile error would point at the
wrong file.

Third, the line structure is preserved exactly. Every line in the generated
file corresponds to a line in the source, at the same or a nearby line
number. print(...) is on line 7 of both files. return 0; is on line 9 of
both. The rewriter does not reflow code, does not combine lines, does not
split lines. It moves tokens, not lines.


## A larger example

The hello-world program is too small to show the language. Here is something
that uses most of cminux's surface, in the shape you might actually write.

    import ["outfn.hpp", "math.hpp"]

    func square(int n) -> int {
        render n * n;
    }

    func describe(int n) -> void {
        if n < 0 {
            println("negative");
        } else {
            println(n);
        }
    }

    func main() -> int {
        let limit = 10;
        var total = 0;

        for var i = 0; i < limit; i = i + 1 {
            total = total + square(i);
        }

        println("sum of squares:");
        println(total);

        var nothing = nil;
        if nothing == nil {
            println("nothing is nothing");
        }

        describe(-1);
        describe(total);

        render 0;
    }

And the translation:

    // generated by cminux v1.0.0 -- do not edit
    // source: example.cmx
    #include "outfn.hpp"
    using cmx::print;
    using cmx::println;

    #line 1 "example.cmx"
    #include "outfn.hpp"
    #include "math.hpp"

    int square(int n) {
        return n * n;
    }

    void describe(int n) {
        if (n < 0) {
            println("negative");
        } else {
            println(n);
        }
    }

    int main() {
        const auto limit = 10;
        auto total = 0;

        for (auto i = 0; i < limit; i = i + 1) {
            total = total + square(i);
        }

        println("sum of squares:");
        println(total);

        auto nothing = nullptr;
        if (nothing == nullptr) {
            println("nothing is nothing");
        }

        describe(-1);
        describe(total);

        return 0;
    }

Every line of the source maps to a line of the output. Nothing was reordered,
nothing was reflowed. func disappeared and the return type moved to the
front. render became return. let became const auto. var became auto. nil
became nullptr. import expanded to two #includes. And if n < 0, which was
if n < 0 in the source, is if (n < 0) in the output -- wait, no. Look again.
The source has if n < 0 with no parens. The output has if (n < 0) with
parens. Something inserted them.

Actually, something did not. In cminux v1.0.0, if is not a keyword. It
passes through unchanged. The source should have been written if (n < 0)
for it to compile -- the parens are part of C++ syntax, and cminux does not
remove them, add them, or change them. The example above is what you get if
you write if n < 0 in cminux source; it fails to compile, because C++
requires the parens. I have left it as-is in the example so you can see the
honest output of the current rewriter, but this is a wart. If you want if to
work without parens in cminux, that is a v1.1 rewrite (strip parens? add
parens? either is a rule in the rewriter) and it is not yet in the
language. Write if (n < 0) in your cminux source until then.


## The output library

cminux's only runtime dependency is a header called outfn.hpp. It declares
print and println, each with overloads for the common types: int, long,
long long, unsigned variants, float, double, bool, char, const char*,
std::string, and std::string_view. It also declares a namespaced printf
that forwards to the C library.

That is the entire library. There is no Vec, no String, no Map, no Result,
no Option. If you want those, use C++'s -- std::vector, std::string,
std::unordered_map, std::optional -- and they work exactly as you expect,
because they are C++ and cminux does not touch them.

The library is small on purpose. Every line of it is a line you have to
maintain, and every function in it is a function that could have been a
line of C++ in your source. The only reason print and println are in the
library at all is that they are so common in introductory programs that
making every program write std::cout << ... << std::endl; would defeat the
purpose of the language. Everything else stays in C++ where it belongs.

If you want to add an overload -- say, print(std::vector<int>) -- add it to
outfn.hpp. Do not add a special case to the transpiler. The transpiler does
not know what a function is and should never learn.


## Installing

cminux is built with CMake. From a Developer Command Prompt on Windows, or
from a shell on macOS and Linux:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

The compiler binary is build\Release\cminux.exe (or build\cminux.exe with a
single-config generator like Ninja). The runtime header is lib\outfn.hpp.

CMake picks cl.exe if you opened a Developer Command Prompt, or g++ /
clang++ if those are on PATH. To force one at build time, pass
-DCMAKE_CXX_COMPILER=<name> to cmake. To force one at runtime, use --cc
<name> or set CMX_CC.


## Using

    cminux hello.cmx -o hello.exe

That is the whole usage. cminux reads hello.cmx, writes hello.cmx.cpp next
to it, invokes the C++ compiler on it, produces hello.exe, and deletes
hello.cmx.cpp unless you passed --keep.

If the C++ compiler fails, cminux forwards the compiler's stderr to your
terminal and exits with a non-zero code. The errors you see are the C++
compiler's errors, on the lines of your .cmx file, thanks to the #line
directives the emitter inserted. cminux does not attempt to translate them
or pretty-print them; C++ compiler errors are usually clear enough once
they point at the right file.

Flags:

    -o, --output <path>     output executable path
    -I <dir>                add include directory (repeatable)
    -C <flag>               pass a flag to the C++ compiler (repeatable)
    --cc <name>             C++ compiler to invoke (overrides CMX_CC)
    --keep                  keep the generated .cmx.cpp
    --tokens                dump the token stream to stderr
    -v, --verbose           print pipeline commands
    -h, --help              show help
    --version               show version


## The language, in full

This is the complete surface of cminux v1.0.0. Everything not listed here is
C++ as-is.

    cminux                          emits
    ------------------------------------------------------------------
    import ["a.hpp", "b.hpp"]       #include "a.hpp"
                                    #include "b.hpp"
    func f() -> T { ... }           T f() { ... }
    func f() { ... }                void f() { ... }
    render e;                       return e;
    nil                             nullptr
    let x = e;                      const auto x = e;
    var x = e;                      auto x = e;
    print(...)                      cmx::print(...)
    println(...)                    cmx::println(...)

That is nine rows. Nine rows is the whole language. If you can remember nine
rows, you know cminux.

Each row is a rule in the rewriter. Adding a row means adding a rule, which
means one more case in src/rewriter.cpp and one more test in tests/. If a
proposed feature cannot be expressed as a row in this table -- if it needs
to know what a type is, or what a name refers to, or what a value equals --
it is not a cminux feature, it is a C++ feature, and it does not belong
here.


## What happens when something goes wrong

The tokenizer fails on unterminated string literals, unterminated block
comments, and characters it does not recognize. It reports the file, line,
and column, and exits with code 1. You will not see these errors from valid
cminux source; you will see them when you typo a quote or accidentally paste
a Unicode character into a string.

The rewriter fails on import without a bracket, import with a non-string
in the list, import with a missing bracket, func without a name, func
without an open paren, func with an unterminated parameter list, func with
an empty return type after ->, and func followed by a forward declaration
(no body). These are all syntax errors in cminux and the error messages say
so. The file, line, and column point at the offending token.

The C++ compiler fails on anything that is a valid cminux program but an
invalid C++ program. Because of the #line directives, the errors point at
your .cmx file with the line numbers you see in your editor. The compiler's
own stderr is forwarded verbatim. cminux does not reformat it, translate it,
or hide it.

If the C++ compiler succeeds but the executable fails at runtime, that is a
runtime failure of your program, not of cminux. cminux did its job -- it
translated and compiled. Debug the program the way you would debug any C++
program: with a debugger, with printf, with the tools you already know.


## Tests

The test suite is in tests/. Each test is a .cmx file plus, optionally, an
expected .cpp, an expected stdout, and an expected exit code. The harness
compiles each, runs it, and compares.

    tests\run.bat

On Windows, or:

    bash tests/run.sh

On macOS and Linux, if you have bash. Both run the same tests against the
same compiler; the only difference is the shell.

Set KEEP=1 to leave artifacts after a run:

    set KEEP=1
    tests\run.bat

Set CMX_BIN to point at a different cminux binary:

    set CMX_BIN=C:\path\to\cminux.exe
    tests\run.bat

A passing run prints one PASS line per test and ends with a summary. A
failing run prints the diff, or the actual stdout vs expected stdout, or the
actual exit code vs expected exit code, and ends with a non-zero exit code.
The test suite is the contract: if tests\run.bat passes, cminux v1.0.0 works.


## What cminux will never do

This section is a promise, and it is the most important section in this
document, because it is what keeps cminux small.

cminux will never parse C++. It will never build an abstract syntax tree. It
will never resolve names, check types, instantiate templates, or generate
code. It will never do optimization, will never do register allocation, will
never do instruction selection. Those are the C++ compiler's job and cminux
will not duplicate them.

cminux will never make C++ safer. It will not add a borrow checker, will not
add a garbage collector, will not add bounds checking, will not add null
safety. If you want those, use a language that has them. cminux makes C++
shorter to write; it does not make it different to reason about.

cminux will never have a package manager. It will never fetch dependencies,
resolve versions, or manage a lockfile. If you want a package manager, use
CMake with FetchContent, or vcpkg, or Conan, or whatever you already use.
cminux compiles one file; it does not know what a project is.

cminux will never have a standard library beyond outfn.hpp. If you want
containers, strings, algorithms, and IO, use the C++ standard library, which
is fully available because cminux emits C++. The reason outfn.hpp exists at
all is that print and println are so common they justify the exception.
Nothing else will join them.

cminux will never be a compiled language. It is a transpiler. It will always
emit .cpp and shell out to a C++ compiler. This is not a stepping stone to
a real compiler; it is the design. The moment cminux emits something other
than portable C++17, it has become something else and lost its reason to
exist.

These promises are not limitations. They are the shape of the language. If
you find yourself wanting cminux to break one of them, you want a different
language, and there are excellent ones out there. Go use them. cminux will
still be here, small and boring, for the 200-line programs it was built for.


## Version

This is cminux v1.0.0. It knows nine rules. It fits in eleven source files.
It has one header of runtime, one test harness, and one README. It compiles
C++ and it compiles fast.

That is the whole thing.