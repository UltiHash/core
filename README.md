# core

## Profiling build process

Prerequisites:

* `sudo apt install ninja-build clang llvm`
* [ClangBuildAnalyzer](https://github.com/aras-p/ClangBuildAnalyzer)

Command:

```bash
rm -rf Debug && \
CC=/usr/bin/clang CXX=/usr/bin/clang++ \
    cmake -G Ninja -S. -BDebug -DCMAKE_BUILD_TYPE=Debug -DTIME_TRACE=ON && \
ClangBuildAnalyzer --start Debug && \
time cmake --build Debug && \
ClangBuildAnalyzer --stop Debug build-profile.bin && \
ClangBuildAnalyzer --analyze build-profile.bin
```

With `-DTIME_TRACE=ON` option you can get json format time trace, 
which is useful to figure out what takes time during compilation of one source file.
Time trace files are next to the regular compiled object files.
You can open this with chrome://tracing or [perfetto](https://ui.perfetto.dev).

ClangBuildAnalyzer aggregates time trace reports from multiple compilations,
and output "what took the most time" summary on console.

