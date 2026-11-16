#include "JsonCore.h"
#include <cstdio>

namespace jsc {
namespace {

bool isWs(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    // Skip a UTF-8 BOM if present.
    if (e >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) b = 3;
    while (b < e && isWs(s[b])) ++b;
    while (e > b && isWs(s[e - 1])) --e;
    return s.substr(b, e - b);
}

std::string where(const std::string& s, size_t pos) {
    size_t line = 1, col = 1;
    for (size_t i = 0; i < pos && i < s.size(); ++i) {
        if (s[i] == '\n') { ++line; col = 1; } else ++col;
    }
    return " (line " + std::to_string(line) + ", column " + std::to_string(col) + ")";
}

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool read4Hex(const std::string& s, size_t p, unsigned& v) {
    if (p + 4 > s.size()) return false;
    v = 0;
    for (size_t k = 0; k < 4; ++k) {
        int h = hexVal(s[p + k]);
        if (h < 0) return false;
        v = (v << 4) | (unsigned)h;
    }
    return true;
}

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) {
        out += (char)cp;
    } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

// Decodes the string literal starting at s[pos] == '"'. On success pos points
// just past the closing quote. Lenient about raw control chars (real newlines
// inside the literal are kept), since pasted strings often contain them.
bool decodeLiteral(const std::string& s, size_t& pos, const Options& opt, std::string& out, std::string& err) {
    if (pos >= s.size() || s[pos] != '"') { err = "Expected '\"'" + where(s, pos); return false; }
    size_t i = pos + 1;
    out.clear();
    while (i < s.size()) {
        char c = s[i];
        if (c == '"') { pos = i + 1; return true; }
        if (c != '\\') { out += c; ++i; continue; }
        if (i + 1 >= s.size()) break;
        char e = s[i + 1];
        switch (e) {
            case '"':  out += '"';  i += 2; break;
            case '\\': out += '\\'; i += 2; break;
            case '/':  out += '/';  i += 2; break;
            case 'b':  out += '\b'; i += 2; break;
            case 'f':  out += '\f'; i += 2; break;
            case 'n':  out += '\n'; i += 2; break;
            case 'r':  out += '\r'; i += 2; break;
            case 't':  out += '\t'; i += 2; break;
            case 'u': {
                unsigned cp;
                if (!read4Hex(s, i + 2, cp)) { err = "Invalid \\u escape" + where(s, i); return false; }
                size_t len = 6;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    unsigned lo;
                    if (i + 7 < s.size() && s[i + 6] == '\\' && s[i + 7] == 'u' && read4Hex(s, i + 8, lo)
                        && lo >= 0xDC00 && lo <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        len = 12;
                    }
                }
                if (cp < 0x80 || opt.utf8) {
                    appendUtf8(out, cp);
                } else {
                    // Non-UTF-8 document: keep the escape so no data is lost.
                    out.append(s, i, len);
                }
                i += len;
                break;
            }
            default:
                err = std::string("Invalid escape '\\") + e + "'" + where(s, i);
                return false;
        }
    }
    err = "Unterminated string (missing closing quote)";
    return false;
}

// ---------------------------------------------------------------- formatter
class Formatter {
public:
    Formatter(const std::string& s, bool pretty, const Options& o) : s_(s), pretty_(pretty), opt_(o) {}

    bool run(std::string& out, std::string& err) {
        skipWs();
        if (!value(0)) { err = err_; return false; }
        skipWs();
        if (i_ != s_.size()) { err = "Unexpected characters after JSON value" + where(s_, i_); return false; }
        out.swap(out_);
        return true;
    }

private:
    const std::string& s_;
    bool pretty_;
    const Options& opt_;
    size_t i_ = 0;
    std::string out_, err_;

    bool fail(const std::string& m) { err_ = m + where(s_, i_); return false; }
    void skipWs() { while (i_ < s_.size() && isWs(s_[i_])) ++i_; }
    void newline(int depth) {
        if (!pretty_) return;
        out_ += opt_.eol;
        for (int k = 0; k < depth; ++k) out_ += opt_.indent;
    }

    bool value(int depth) {
        if (depth > 1000) return fail("JSON nested too deeply");
        if (i_ >= s_.size()) return fail("Unexpected end of input");
        char c = s_[i_];
        if (c == '{') return container(depth, '{', '}', true);
        if (c == '[') return container(depth, '[', ']', false);
        if (c == '"') return string();
        if (c == '-' || (c >= '0' && c <= '9')) return number();
        if (literal("true") || literal("false") || literal("null")) return true;
        return fail(std::string("Unexpected character '") + c + "'");
    }

    bool literal(const char* w) {
        size_t n = std::char_traits<char>::length(w);
        if (s_.compare(i_, n, w) != 0) return false;
        out_.append(w, n);
        i_ += n;
        return true;
    }

    bool container(int depth, char open, char close, bool isObject) {
        out_ += open;
        ++i_;
        skipWs();
        if (i_ < s_.size() && s_[i_] == close) { out_ += close; ++i_; return true; }
        for (;;) {
            newline(depth + 1);
            if (isObject) {
                skipWs();
                if (i_ >= s_.size() || s_[i_] != '"') return fail("Expected property name");
                if (!string()) return false;
                skipWs();
                if (i_ >= s_.size() || s_[i_] != ':') return fail("Expected ':'");
                ++i_;
                out_ += pretty_ ? ": " : ":";
                skipWs();
            }
            if (!value(depth + 1)) return false;
            skipWs();
            if (i_ >= s_.size()) return fail(std::string("Expected ',' or '") + close + "'");
            if (s_[i_] == ',') { out_ += ','; ++i_; skipWs(); continue; }
            if (s_[i_] == close) { newline(depth); out_ += close; ++i_; return true; }
            return fail(std::string("Expected ',' or '") + close + "'");
        }
    }

    // Copies a string literal verbatim (escapes untouched) after validating it.
    bool string() {
        size_t start = i_++;
        while (i_ < s_.size()) {
            unsigned char c = (unsigned char)s_[i_];
            if (c == '"') { ++i_; out_.append(s_, start, i_ - start); return true; }
            if (c < 0x20) return fail("Control character inside string (should be escaped)");
            if (c == '\\') {
                if (i_ + 1 >= s_.size()) break;
                char e = s_[i_ + 1];
                if (e == 'u') {
                    unsigned v;
                    if (!read4Hex(s_, i_ + 2, v)) return fail("Invalid \\u escape");
                    i_ += 6;
                } else if (e == '"' || e == '\\' || e == '/' || e == 'b' || e == 'f' || e == 'n' || e == 'r' || e == 't') {
                    i_ += 2;
                } else {
                    return fail(std::string("Invalid escape '\\") + e + "'");
                }
                continue;
            }
            ++i_;
        }
        i_ = start;
        return fail("Unterminated string");
    }

    bool number() {
        size_t start = i_;
        auto digits = [&] { size_t b = i_; while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9') ++i_; return i_ > b; };
        if (s_[i_] == '-') ++i_;
        if (i_ < s_.size() && s_[i_] == '0') ++i_;
        else if (!digits()) return fail("Invalid number");
        if (i_ < s_.size() && s_[i_] == '.') { ++i_; if (!digits()) return fail("Invalid number"); }
        if (i_ < s_.size() && (s_[i_] == 'e' || s_[i_] == 'E')) {
            ++i_;
            if (i_ < s_.size() && (s_[i_] == '+' || s_[i_] == '-')) ++i_;
            if (!digits()) return fail("Invalid number");
        }
        out_.append(s_, start, i_ - start);
        return true;
    }
};

// True if t is exactly one complete string literal.
bool isWholeLiteral(const std::string& t, const Options& opt, std::string& decoded) {
    if (t.size() < 2 || t[0] != '"') return false;
    size_t p = 0;
    std::string err;
    return decodeLiteral(t, p, opt, decoded, err) && p == t.size();
}

} // namespace

bool unescapeJsonString(const std::string& in, const Options& opt, std::string& out, std::string& err) {
    std::string t = trim(in);
    if (t.empty()) { err = "Nothing to convert"; return false; }

    std::string decoded;
    if (!isWholeLiteral(t, opt, decoded)) {
        // Allow input without the surrounding quotes, e.g.  {\"a\":1}
        if (t.find('\\') == std::string::npos) { err = "Text is not an escaped JSON string (no backslash escapes found)"; return false; }
        std::string wrapped = "\"" + t + "\"";
        size_t p = 0;
        if (!decodeLiteral(wrapped, p, opt, decoded, err)) return false;
        if (p != wrapped.size()) { err = "Found an unescaped '\"' inside the string" + where(wrapped, p - 1); return false; }
    }

    // Peel off further layers of escaping (double-encoded strings).
    for (int layer = 0; layer < 16; ++layer) {
        std::string next, tt = trim(decoded);
        if (!isWholeLiteral(tt, opt, next)) break;
        decoded.swap(next);
    }
    out.swap(decoded);
    return true;
}

bool formatJson(const std::string& in, bool pretty, const Options& opt, std::string& out, std::string& err) {
    std::string t = trim(in);
    if (t.empty()) { err = "Nothing to format"; return false; }
    Formatter f(t, pretty, opt);
    return f.run(out, err);
}

std::string escapeToJsonString(const std::string& in) {
    std::string out = "\"";
    for (unsigned char c : in) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    out += '"';
    return out;
}

} // namespace jsc
