// Pure C++ JSON helpers (no Windows dependencies) used by the plugin.
#pragma once
#include <string>

namespace jsc {

struct Options {
    bool        utf8   = true;     // document is UTF-8: decode \uXXXX to real characters
    std::string indent = "    ";   // indentation unit for pretty printing
    std::string eol    = "\r\n";   // line ending for pretty printing
};

// Turns a JSON *string* such as  "{\"a\":1,\"b\":\"x\"}"  (with or without the
// surrounding quotes) back into the JSON text it contains. Repeats while the
// result is itself a string literal (handles double/triple-escaped input).
// Returns false and sets err on failure.
bool unescapeJsonString(const std::string& in, const Options& opt, std::string& out, std::string& err);

// Re-formats valid JSON. pretty=false minifies. Returns false and sets err if
// the input is not valid JSON.
bool formatJson(const std::string& in, bool pretty, const Options& opt, std::string& out, std::string& err);

// The reverse operation: wraps text into a JSON string literal ("..." with
// quotes, backslashes and control characters escaped).
std::string escapeToJsonString(const std::string& in);

} // namespace jsc
