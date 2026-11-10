// src/main.cpp
// cminux command-line driver.

#include "driver.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

namespace {

void print_usage(std::ostream& out) {
    out <<
        "cminux v1.1.0 - a simpler C++ that compiles to native executables\n"
        "\n"
        "usage: cminux <file.cmx> [options]\n"
        "\n"
        "options:\n"
        "  -o, --output <path>     output executable path\n"
        "  -I <dir>                add include directory (repeatable)\n"
        "  -C <flag>               pass a flag to the C++ compiler (repeatable)\n"
        "  --cc <name>             C++ compiler to invoke (overrides CMX_CC)\n"
        "  --keep                  keep the generated .cpp next to the input\n"
        "  --tokens                dump the token stream to stderr\n"
        "  -v, --verbose           print pipeline commands\n"
        "  -h, --help              show this help\n"
        "  --version               show the version and exit\n"
        "\n"
        "environment:\n"
        "  CMX_CC                  C++ compiler to invoke (default: platform)\n"
        "  CMX_LIB_DIR             directory containing outfn.hpp\n";
}

} // namespace

int main(int argc, char** argv) {
    cmx::DriverOptions opts;

    if (argc <= 1) {
        print_usage(std::cerr);
        return 1;
    }

    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need_arg = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "cminux: " << what << " requires an argument\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if (a == "-h" || a == "--help") {
            print_usage(std::cout);
            return 0;
        }
        if (a == "--version") {
            std::cout << "cminux 1.0.0\n";
            return 0;
        }
        if (a == "-o" || a == "--output") {
            opts.output_exe = need_arg("--output");
            continue;
        }
        if (a == "-I") {
            opts.include_dirs.push_back(need_arg("-I"));
            continue;
        }
        if (a == "-C") {
            opts.extra_flags.push_back(need_arg("-C"));
            continue;
        }
        if (a == "--cc") {
            opts.compiler = need_arg("--cc");
            continue;
        }
        if (a == "--keep") { opts.keep_cpp = true; continue; }
        if (a == "--tokens") { opts.dump_tokens = true; continue; }
        if (a == "-v" || a == "--verbose") { opts.verbose = true; continue; }

        if (!a.empty() && a[0] == '-') {
            std::cerr << "cminux: unknown option '" << a << "'\n";
            return 1;
        }
        positional.push_back(a);
    }

    if (positional.size() != 1) {
        std::cerr << "cminux: expected exactly one input file\n";
        return 1;
    }
    opts.input_path = positional[0];

    return cmx::run(opts);
}