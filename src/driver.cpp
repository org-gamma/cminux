// src/driver.cpp
// The full cminux pipeline for one source file.

#include "driver.hpp"
#include "tokenizer.hpp"
#include "rewriter.hpp"
#include "emitter.hpp"
#include "compiler.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace cmx {

namespace {

std::string read_file(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { ok = false; return {}; }
    std::ostringstream buf;
    buf << in.rdbuf();
    ok = true;
    return buf.str();
}

bool write_file(const std::string& path, const std::string& contents) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << contents;
    return static_cast<bool>(out);
}

// Generate the temp .cpp name next to the input.
std::string temp_cpp_path(const std::string& input_path) {
    size_t slash = input_path.find_last_of("/\\");
    std::string dir  = (slash == std::string::npos) ? "" : input_path.substr(0, slash + 1);
    std::string base = (slash == std::string::npos) ? input_path : input_path.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    std::string stem = (dot == std::string::npos) ? base : base.substr(0, dot);
    return dir + stem + ".cmx.cpp";
}

std::string derive_exe_path(const std::string& input_path) {
    size_t slash = input_path.find_last_of("/\\");
    std::string dir  = (slash == std::string::npos) ? "" : input_path.substr(0, slash + 1);
    std::string base = (slash == std::string::npos) ? input_path : input_path.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    std::string stem = (dot == std::string::npos) ? base : base.substr(0, dot);
#if defined(_WIN32)
    return dir + stem + ".exe";
#else
    return dir + stem;
#endif
}

void report_error(const std::string& file, int line, int col, const std::string& msg) {
    std::cerr << file << ":" << line << ":" << col << ": error: " << msg << "\n";
}

} // namespace

int run(const DriverOptions& opts) {
    bool ok = false;
    std::string source = read_file(opts.input_path, ok);
    if (!ok) {
        std::cerr << "cminux: cannot open '" << opts.input_path << "'\n";
        return 1;
    }

    // ---- tokenize --------------------------------------------------------
    std::vector<Token> toks;
    TokenizeError terr;
    if (!tokenize(source, opts.input_path, toks, terr)) {
        report_error(opts.input_path, terr.line, terr.col, terr.message);
        return 1;
    }

    if (opts.dump_tokens) {
        for (size_t i = 0; i < toks.size(); ++i) {
            const auto& t = toks[i];
            const char* k = "";
            switch (t.kind) {
                case TokKind::End:     k = "END";   break;
                case TokKind::Newline: k = "NL";    break;
                case TokKind::Keyword: k = "KW";    break;
                case TokKind::Ident:   k = "IDENT"; break;
                case TokKind::String:  k = "STR";   break;
                case TokKind::Number:  k = "NUM";   break;
                case TokKind::Punct:   k = "PUNCT"; break;
            }
            std::cerr << i << " " << k << " "
                      << t.line << ":" << t.col << " '" << t.text << "'\n";
        }
    }

    // ---- rewrite ---------------------------------------------------------
    std::vector<Token> rewritten;
    RewriteError rerr;
    if (!rewrite(toks, rewritten, rerr)) {
        report_error(opts.input_path, rerr.line, rerr.col, rerr.message);
        return 1;
    }

    // ---- emit ------------------------------------------------------------
    EmitOptions eopts;
    eopts.source_filename     = opts.input_path;
    eopts.emit_line_directive = true;
    eopts.emit_prologue       = true;
    std::string cpp = emit_cpp(rewritten, eopts);

    std::string tmp = temp_cpp_path(opts.input_path);
    if (!write_file(tmp, cpp)) {
        std::cerr << "cminux: cannot write '" << tmp << "'\n";
        return 1;
    }

    // ---- compile ---------------------------------------------------------
    CompileOptions copts;
    copts.cpp_source  = tmp;
    copts.output_exe  = opts.output_exe.empty() ? derive_exe_path(opts.input_path) : opts.output_exe;
    copts.include_dirs = opts.include_dirs;
    copts.extra_flags  = opts.extra_flags;
    copts.compiler     = opts.compiler;
    copts.verbose      = opts.verbose;

    // Resolve `#include "outfn.hpp"` from anywhere. Prefer the environment
    // variable, fall back to the compile-time path baked in by CMake.
    if (const char* libdir = std::getenv("CMX_LIB_DIR")) {
        if (libdir[0] != '\0') copts.include_dirs.push_back(libdir);
    } else {
#ifndef CMX_LIB_DIR_DEFAULT
#define CMX_LIB_DIR_DEFAULT "lib"
#endif
        copts.include_dirs.push_back(CMX_LIB_DIR_DEFAULT);
    }

    std::string cerr_msg;
    if (!compile_to_exe(copts, cerr_msg)) {
        std::cerr << "cminux: " << cerr_msg << "\n";
        if (!opts.keep_cpp) std::remove(tmp.c_str());
        return 1;
    }

    if (!opts.keep_cpp) {
        std::remove(tmp.c_str());
    } else if (opts.verbose) {
        std::cerr << "cminux: kept " << tmp << "\n";
    }

    if (opts.verbose) {
        std::cerr << "cminux: built " << copts.output_exe << "\n";
    }
    return 0;
}

} // namespace cmx