#include "../src/JsonCore.h"
#include <cstdio>
#include <string>

static int fails = 0;
static void check(bool ok, const char* name, const std::string& got = "") {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) { std::printf("   got: [%s]\n", got.c_str()); ++fails; }
}

int main() {
    jsc::Options o; o.eol = "\n"; o.indent = "  ";
    std::string out, err;

    bool ok = jsc::unescapeJsonString("\"{\\\"a\\\":1,\\\"b\\\":[true,null,\\\"x\\\\ny\\\"]}\"", o, out, err);
    check(ok && out == "{\"a\":1,\"b\":[true,null,\"x\\ny\"]}", "quoted string", out + err);

    ok = jsc::unescapeJsonString("  {\\\"a\\\":\\\"\\u00e9\\\"}  ", o, out, err);
    check(ok && out == "{\"a\":\"\xC3\xA9\"}", "no quotes + \\u", out + err);

    ok = jsc::unescapeJsonString("\"\\\"{\\\\\\\"a\\\\\\\":1}\\\"\"", o, out, err);
    check(ok && out == "{\"a\":1}", "double escaped", out + err);

    ok = jsc::unescapeJsonString("\"\\ud83d\\ude00\"", o, out, err);
    check(ok && out == "\xF0\x9F\x98\x80", "surrogate pair", out + err);

    jsc::Options ansi = o; ansi.utf8 = false;
    ok = jsc::unescapeJsonString("{\\\"a\\\":\\\"\\u00e9\\\"}", ansi, out, err);
    check(ok && out == "{\"a\":\"\\u00e9\"}", "ansi keeps \\u", out + err);

    ok = jsc::unescapeJsonString("{\"a\":1}", o, out, err);
    check(!ok, "plain json rejected", err);

    ok = jsc::formatJson("{\"a\":1,\"b\":[1,2,{}],\"c\":{\"d\":\"e\\\"f\"},\"e\":[]}", true, o, out, err);
    check(ok && out == "{\n  \"a\": 1,\n  \"b\": [\n    1,\n    2,\n    {}\n  ],\n  \"c\": {\n    \"d\": \"e\\\"f\"\n  },\n  \"e\": []\n}", "pretty", out + err);

    ok = jsc::formatJson(" { \"a\" : [ 1 , -2.5e3 ] } ", false, o, out, err);
    check(ok && out == "{\"a\":[1,-2.5e3]}", "minify", out + err);

    ok = jsc::formatJson("{\"a\":1,}", true, o, out, err);
    check(!ok, "invalid json detected", err);
    std::printf("   err: %s\n", err.c_str());

    std::string esc = jsc::escapeToJsonString("{\"a\":\"x\\ny\"}\n");
    check(esc == "\"{\\\"a\\\":\\\"x\\\\ny\\\"}\\n\"", "escape", esc);
    ok = jsc::unescapeJsonString(esc, o, out, err);
    check(ok && out == "{\"a\":\"x\\ny\"}\n", "round trip", out);

    std::printf("\n%d failure(s)\n", fails);
    return fails;
}
