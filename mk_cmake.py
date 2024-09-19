#!/usr/bin/env python3

import os
import re

def grep(file, pattern):
    rv = []
    m = re.compile(pattern)
    with open(file) as f:
        for _, line in enumerate(f):
            if m.match(line) is not None:
                rv.append(line)

    return rv

def is_dep_fp(dependency):
    return dependency in [
        "uh_recovery",
        "uh_otlp",
        "uh_view",
        "uh_std",
        "uh_trace",
        "uh_propagation",
        "uh_debug",
        "uh_coroutines",
        "uh_interfaces",
        "uh_etcd",
        "uh_service_discovery",
        "uh_caches",
    ]

def dependencies(files):
    m = re.compile('#include "([^"]+)"')
    ipaths = dict()
    zpp_bits = False
    lmdbxx = False
    magic_enum = False
    otel = False
    nlohmann = False
    reed = False
    cli11 = False
    pgsql = False
    for f in files:
        includes = grep(f, '^#include "')
        for inc in includes:
            mg = m.match(inc)
            if mg is not None:
                ipaths[mg.group(1)] = True

        if not zpp_bits:
            zpp_bits = len(grep(f, '.*zpp_bits.h.*')) > 0
        if not lmdbxx:
            lmdbxx = len(grep(f, '.*lmdbxx.*')) > 0
        if not magic_enum:
            magic_enum = len(grep(f, '#include.*magic_enum.*')) > 0
        if not otel:
            otel = len(grep(f, '#include.*opentelemetry.*')) > 0
        if not nlohmann:
            nlohmann = len(grep(f, '#include.*nlohmann.*')) > 0
        if not reed:
            reed = len(grep(f, '#include.*rs.h.*')) > 0
        if not cli11:
            cli11 = len(grep(f, '#include.*CLI/CLI.hpp.*')) > 0
        if not pgsql:
            pgsql = len(grep(f, '#include.*libpq-fe.*')) > 0


    dirs = [os.path.dirname(f) for f in ipaths.keys()]
    libs = ["uh_" + os.path.basename(f) for f in dirs if f != ""]
    libs = [x for x in libs if not is_dep_fp(x)]
    libs = list(set(libs))
    if zpp_bits:
        libs.append('zpp_bits')
    if lmdbxx:
        libs.append('lmdbxx')
    if magic_enum:
        libs.append('magic_enum')
    if otel:
        libs += ['opentelemetry_exporter_otlp_grpc_log', 'opentelemetry_exporter_otlp_grpc', 'opentelemetry_exporter_otlp_grpc_metrics', 'opentelemetry_exporter_ostream_metrics']
    if nlohmann:
        libs.append('nlohmann_json')
    if reed:
        libs.append('reedsolomon')
    if cli11:
        libs.append('CLI11::CLI11')
    if pgsql:
        libs.append('PostgreSQL::PostgreSQL')
    return libs

def generate_cmake(path):
    library = os.path.basename(path)
    files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    dirs = [f for f in os.listdir(path) if os.path.isdir(os.path.join(path, f))]

    h_files = [f for f in files if os.path.splitext(f)[1] == '.h']
    cpp_files = [f for f in files if os.path.splitext(f)[1] == '.cpp']
    cpp_string = " ".join(cpp_files)

    libs = dependencies([os.path.join(path, f) for f in cpp_files + h_files])

    with open(os.path.join(path, "CMakeLists.txt"), 'x') as f:
        if len(cpp_files) != 0:
            f.write(f"add_library(uh_{library} {cpp_string})\n")
            if len(libs) != 0:
                libs = [f for f in libs if f != f"uh_{library}"]
                lib_string = " ".join(libs)
                f.write(f"target_link_libraries(uh_{library} {lib_string})\n")

        if len(dirs) != 0:
            for d in dirs:
                f.write(f"add_subdirectory({d})\n")

def generate_exec_cmake(path, name):
    files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    dirs = [f for f in os.listdir(path) if os.path.isdir(os.path.join(path, f))]

    h_files = [f for f in files if os.path.splitext(f)[1] == '.h']
    cpp_files = [f for f in files if os.path.splitext(f)[1] == '.cpp']
    cpp_string = " ".join(cpp_files)

    libs = dependencies([os.path.join(path, f) for f in cpp_files + h_files])

    with open(os.path.join(path, "CMakeLists.txt"), 'x') as f:
        f.write("include_directories(${CMAKE_CURRENT_SOURCE_DIR})\n")
        f.write("include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)\n")
        f.write("include_directories(${Boost_INCLUDE_DIRS})\n")
        if len(cpp_files) != 0:
            f.write(f"add_executable({name} {cpp_string})\n")
            if len(libs) != 0:
                lib_string = " ".join(libs)
                f.write(f"target_link_libraries({name} {lib_string})\n")

        if len(dirs) != 0:
            for d in dirs:
                f.write(f"add_subdirectory({d})\n")


if __name__ == "__main__":
    for root, dirs, files in os.walk("src"):
        for d in dirs:
            dir_path = os.path.join(root, d)
            print(f"{dir_path}")

            if os.path.exists(dir_path.join("CMakeLists.txt")):
                print(f"{dir_path}/CMakeLists.txt found, skipping")
            else:
                generate_cmake(dir_path)

    generate_exec_cmake("/home/sjank/projects/core/src", "uh-cluster-2")
