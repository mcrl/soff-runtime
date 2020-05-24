#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 [opae | xrt]"
  exit 1
fi

if [ "$1" != "opae" ] && [ "$1" != "xrt" ]; then
  echo "Usage: $0 [opae | xrt]"
  exit 1
fi

if [ -z "$SOFF_PREFIX" ]; then
  echo "Define the SOFF_PREFIX environment variable before running $0."
  exit 1
fi

if [ ! -d "$SOFF_PREFIX" ]; then
  echo "SOFF_PREFIX=$SOFF_PREFIX is not a valid directory.";
  exit 1
fi

SNUCLROOT="$PWD"
if [ ! -d "$SNUCLROOT/inc" ] || [ ! -d "$SNUCLROOT/runtime" ]; then
  echo "The current directory does not contains soff-runtime."
  exit 1
fi
if [ ! -f "$SNUCLROOT/runtime/Makefile.$1" ] || [ ! -d "$SNUCLROOT/runtime/$1" ]; then
  echo "The current directory does not contains the runtime implementation for $1."
  exit 1
fi

echo "[SOFF] Compiling the runtime..."
cd "$SNUCLROOT/runtime"
SNUCLROOT=$SNUCLROOT make -j -f Makefile.$1

echo "[SOFF] Copying files to $SOFF_PREFIX..."
mkdir -p "$SOFF_PREFIX/lib"
cp "$SNUCLROOT/runtime/libsoff_opae.so" "$SOFF_PREFIX/lib/"
mkdir -p "$SOFF_PREFIX/include"
cp -r "$SNUCLROOT/inc/"* "$SOFF_PREFIX/include/"

echo "[SOFF] soff-runtime installation complete."
