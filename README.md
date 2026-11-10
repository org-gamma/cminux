# cminux

A simpler C++ that compiles to native executables.

cminux is a source-to-source translator. You write .cmx files, cminux
emits C++17, invokes your C++ compiler, and hands you an executable.
There is no interpreter, no runtime, no garbage collector. The output is
ordinary C++ and compiles on any C++17 compiler.

## Build

    run.bat                              # Windows
    ./run.sh                             # macOS / Linux

The compiler is build/cminux(.exe). The library header is lib/outfn.hpp.

## Use

    cminux hello.cmx -o hello.exe
    hello.exe

## The language

Nine rules. Everything else is C++ as-is.

| cminux                      | emits                                       |
|-----------------------------|---------------------------------------------|
| import ["a.hpp", "b.hpp"]   | #include "a.hpp" / #include "b.hpp"         |
| func f() -> T { ... }       | T f() { ... }                               |
| func f() { ... }            | void f() { ... }                            |
| render e;                   | return e;                                   |
| nil                         | nullptr                                     |
| let x = e;                  | const auto x = e;                           |
| var x = e;                  | auto x = e;                                 |
| print(...)                  | cmx::print(...)   (from outfn.hpp)          |
| println(...)                | cmx::println(...) (from outfn.hpp)          |

## Example

    import ["outfn.hpp"]

    func add(int a, int b) -> int {
        render a + b;
    }

    func main() -> int {
        let x = add(40, 2);
        println(x);
        render 0;
    }

## Flags

    -o, --output <path>   output executable
    -I <dir>              extra include directory
    -C <flag>             extra C++ compiler flag
    --cc <name>           compiler to invoke
    --keep                keep the generated .cmx.cpp
    --tokens              dump the token stream
    -v, --verbose         print pipeline commands

## Tests

    tests\run.bat         Windows
    bash tests/run.sh     macOS / Linux

## License

GNU Affero General Public License v3