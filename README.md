[![Build and test](https://github.com/ClementDreptin/XBDM/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/ClementDreptin/XBDM/actions/workflows/build-and-test.yml)

# XBDM
Cross-platform client for the XBDM protocol used by development versions of the Xbox 360.

## Compiling

### Generating the project files / Makefiles

Windows
- Visual Studio
    ```
    .\scripts\genprojects-win-vs.bat
    ```
- Cygwin / MinGW
    ```
    .\scripts\genprojects-win.bat
    ```

Linux / macOS
```
./scripts/genprojects-posix.sh
```

**Note:** If you want to build the tests, just pass `--test` to the script to generate the project files / Makefiles. For example:
```
./scripts/genprojects-posix.sh --test
```

### Building

Windows
- Visual Studio
    ```
    Open .\build\XBDM.sln in Visual Studio
    ```
    or, if you have `msbuild` in your `PATH`
    ```
    msbuild /p:Configuration=<debug|release> .\build\XBDM.sln
    ```
- Cygwin / MinGW
    ```
    cd build && make config=<debug|release>
    ```

Linux / macOS
```
cd build && make config=<debug|release>
```
