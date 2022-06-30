#!/bin/bash

SCRIPT_DIR=`cd \`dirname "$BASH_SOURCE"\` && pwd -P`
ROOT_DIR=`dirname "$SCRIPT_DIR"`
PREMAKE_EXECUTABLE_PATH=$ROOT_DIR/tools/premake/premake5

if [[ ! -f "$PREMAKE_EXECUTABLE_PATH" ]]; then
    echo "Downloading premake..."
    "$SCRIPT_DIR/download-premake-posix.sh"
fi

if [[ $1 == "--test" ]]; then
    "$PREMAKE_EXECUTABLE_PATH" --file="$ROOT_DIR/premake5.lua" gmake2 --test
else
    "$PREMAKE_EXECUTABLE_PATH" --file="$ROOT_DIR/premake5.lua" gmake2
fi
