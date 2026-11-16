#!/bin/sh
# Cross-compile the plugin with MinGW (Linux). Produces x64 and x86 DLLs + release zips.
set -e
cd "$(dirname "$0")"
VERSION=1.0.0.0
FLAGS="-std=c++17 -O2 -shared -static -static-libgcc -static-libstdc++ -Wl,--kill-at -DUNICODE -D_UNICODE"
for ARCH in x64 x86; do
  if [ $ARCH = x64 ]; then P=x86_64-w64-mingw32; else P=i686-w64-mingw32; fi
  OUT=dist/$ARCH/JsonStringConverter
  mkdir -p $OUT release
  $P-windres src/version.rc -O coff -o dist/version_$ARCH.o
  $P-g++ $FLAGS -o $OUT/JsonStringConverter.dll src/Plugin.cpp src/JsonCore.cpp dist/version_$ARCH.o
  $P-strip $OUT/JsonStringConverter.dll
  rm -f dist/version_$ARCH.o release/JsonStringConverter_v${VERSION}_$ARCH.zip
  # Plugins Admin expects the DLL at the root of the zip.
  (cd $OUT && zip -q -X ../../../release/JsonStringConverter_v${VERSION}_$ARCH.zip JsonStringConverter.dll)
done
sha256sum release/*.zip
