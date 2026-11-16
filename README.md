# JSON String Converter – Notepad++ plugin

Turns an escaped JSON string into real, formatted JSON.

Before:
```
"{\"name\":\"John\",\"tags\":[\"a\",\"b\"],\"address\":{\"city\":\"Paris\"}}"
```
After (Plugins → JSON String Converter → JSON string -> JSON (formatted), or **Ctrl+Alt+Shift+J**):
```json
{
    "name": "John",
    "tags": [
        "a",
        "b"
    ],
    "address": {
        "city": "Paris"
    }
}
```

## Menu commands
| Command | What it does |
|---|---|
| JSON string -> JSON (formatted) | Unescapes and pretty-prints. Shortcut Ctrl+Alt+Shift+J |
| JSON string -> JSON (unescape only) | Unescapes and leaves the layout as is |
| Pretty print JSON | Formats JSON with 4-space indentation |
| Minify JSON | Removes all whitespace |
| JSON -> JSON string (escape) | The reverse: turns JSON into an escaped string |

* Works on the selection, or the whole document when nothing is selected.
* Surrounding quotes are optional (`{\"a\":1}` works too).
* Double- or triple-escaped strings are unwrapped completely.
* `\uXXXX` escapes become real characters in UTF-8 documents.
* If the result isn't valid JSON, you get a message with line/column.
* Every change is one Undo step (Ctrl+Z).

## Install
1. Download `JsonStringConverter_v…_x64.zip` (64-bit Notepad++, the usual one) or `…_x86.zip` (32-bit)
   from the [Releases page](https://github.com/meysam-mahdavirad/JsonStringConverter/releases/latest).
2. Close Notepad++.
3. Create the folder `C:\Program Files\Notepad++\plugins\JsonStringConverter\` and extract
   `JsonStringConverter.dll` into it.
4. Start Notepad++. The plugin is in the **Plugins** menu.

Not sure which one you have? Check **? → About Notepad++** (it says 32-bit or 64-bit).

## Build from source
* Linux (MinGW): `./build.sh` (DLLs in `dist/`, release zips in `release/`)
* Windows (Visual Studio): `cmake -B build -A x64 && cmake --build build --config Release`
* Core tests: `g++ -std=c++17 tests/test_core.cpp src/JsonCore.cpp -o test && ./test`

## License
MIT – see [LICENSE](LICENSE).
