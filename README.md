# core

## Profiling build process of single file

Prerequisites:

* `sudo apt install ninja-build clang llvm`
* [ClangBuildAnalyzer](https://github.com/aras-p/ClangBuildAnalyzer)
* [externis](https://github.com/royjacobson/externis.git)
    * Install gcc-13-plugin-dev & libfmt-dev before

Command:

```bash
BTYPE=RelWithDebInfo; COMPILE_OUTPUT=clang-profile.bin; NINJA_OUTPUT=ninja-profile.json;\
    rm -rf ${BTYPE} && \
    CC=/usr/bin/clang CXX=/usr/bin/clang++ \
        cmake -G Ninja -S. -B${BTYPE} -DCMAKE_BUILD_TYPE=${BTYPE} -DENABLE_BUILD_TIME_TRACE=ON && \
    ClangBuildAnalyzer --start ${BTYPE} && \
    time cmake --build ${BTYPE} && \
    ClangBuildAnalyzer --stop ${BTYPE} ${COMPILE_OUTPUT} && \
    ClangBuildAnalyzer --analyze ${COMPILE_OUTPUT} && \
    ninjatracing ${BTYPE}/.ninja_log > ${NINJA_OUTPUT}
```

With GCC:


```bash
BTYPE=RelWithDebInfo; GCC_OUTPUT=gcc-profile.bin; NINJA_OUTPUT=build-profile.json;\
    rm -rf ${BTYPE} && \
    cmake -G Ninja -S. -B${BTYPE} -DCMAKE_BUILD_TYPE=${BTYPE} -DENABLE_BUILD_TIME_TRACE=ON && \
    time cmake --build ${BTYPE} && \
    ninjatracing ${BTYPE}/.ninja_log > ${NINJA_OUTPUT}
```

With `-DENABLE_BUILD_TIME_TRACE=ON` option you can get json format time trace, 
which is useful to figure out what takes time during compilation of one source file.
Time trace files are next to the regular compiled object files.
You can open this with chrome://tracing or [perfetto](https://ui.perfetto.dev).

ClangBuildAnalyzer aggregates time trace reports from multiple compilations,
and output "what took the most time" summary on console.

