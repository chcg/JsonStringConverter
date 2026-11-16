# Publishing to Notepad++ Plugins Admin

Entries below are filled in for github.com/meysam-mahdavirad/jsonstringconverter.

## 1. Put the source on GitHub
Done: https://github.com/meysam-mahdavirad/jsonstringconverter (MIT license).

## 2. Create a release
Releases → Draft a new release → tag `v1.0.0.0` → attach both files from `release/`:
- `JsonStringConverter_v1.0.0.0_x64.zip`
- `JsonStringConverter_v1.0.0.0_x86.zip`

Upload these exact files: the `id` below is their SHA-256. If you rebuild, re-run `sha256sum release/*.zip` and update the ids.

## 3. Open a PR to the official list
Fork https://github.com/notepad-plus-plus/nppPluginList and add one entry to each file
(the list is alphabetical by display-name, so insert it in order).

`src/pl.x64.json`
```json
{
	"folder-name": "JsonStringConverter",
	"display-name": "JSON String Converter",
	"version": "1.0.0.0",
	"npp-compatible-versions": "[8.0,]",
	"id": "92229735a5efdb06f736bde3921b0a709c065b55955e0b58905811907f2cf266",
	"repository": "https://github.com/meysam-mahdavirad/jsonstringconverter/releases/download/v1.0.0.0/JsonStringConverter_v1.0.0.0_x64.zip",
	"description": "Converts an escaped JSON string (e.g. \"{\\\"a\\\":1}\") into formatted JSON. Handles strings with or without surrounding quotes and double-escaped strings. Also pretty-prints, minifies and escapes JSON. Works on the selection or the whole document. Shortcut: Ctrl+Alt+Shift+J.",
	"author": "Meysam Mahdavirad",
	"homepage": "https://github.com/meysam-mahdavirad/jsonstringconverter"
},
```

`src/pl.x86.json`: the same, but with
```json
	"id": "60c212b7bd95900bc95fb645f94afb2d2944a1b8a9c29afd5cc21b87f166ae30",
	"repository": "https://github.com/meysam-mahdavirad/jsonstringconverter/releases/download/v1.0.0.0/JsonStringConverter_v1.0.0.0_x86.zip",
```

PR title: `Add JSON String Converter 1.0.0.0`. Their CI downloads the zips and checks the hash,
the DLL name (= folder-name) and the DLL version (= version). Once a maintainer merges it, the
plugin shows up in Plugins Admin with the next plugin-list update.

## Releasing an update later
1. Bump the version in `src/version.rc` (both numeric and string fields) and `build.sh`.
2. `./build.sh`, create a new GitHub release with the new zips.
3. PR to nppPluginList updating `version`, `id` and `repository`.
