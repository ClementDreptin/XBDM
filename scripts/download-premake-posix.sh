#!/bin/bash

SCRIPT_DIR=`cd \`dirname "$BASH_SOURCE"\` && pwd -P`
ROOT_DIR=`dirname "$SCRIPT_DIR"`
PREMAKE_DIR=$ROOT_DIR/tools/premake
PREMAKE_ARCHIVE_PATH=$PREMAKE_DIR/tmp.zip
PREMAKE_VERSION=5.0.0-beta2
OS_LABEL=""

# Check the OS type
if [[ $OSTYPE == "linux"* ]]; then
    OS_LABEL=linux
elif [[ $OSTYPE == "darwin"* ]]; then
    OS_LABEL=macosx
fi

# If the OS is not Linux nor macOS, exit
if [[ $OS_LABEL == "" ]]; then
    echo "Operating system is not supported!"
    exit 1
fi

# Download the premake archive from GitHub
wget -q -O "$PREMAKE_ARCHIVE_PATH" https://github.com/premake/premake-core/releases/download/v$PREMAKE_VERSION/premake-$PREMAKE_VERSION-$OS_LABEL.tar.gz

# Extract the executable from the archive
tar -xzf "$PREMAKE_ARCHIVE_PATH" -C "$PREMAKE_DIR"

# Delete the premake archive
rm "$PREMAKE_ARCHIVE_PATH"
