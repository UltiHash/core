#!/usr/bin/env python3

import os

def generate_cmake(path, files):
    library = os.path.basename(path)
    filestring = " ".join(files)
    with open(os.path.join(path, "CMakeLists.txt"), 'x') as f:
        f.write(f"add_library({library} {filestring})")


for root, dirs, files in os.walk("src"):
    for d in dirs:
        dir_path = os.path.join(root, d)
        print(f"{dir_path}")

        if os.path.exists(dir_path.join("CMakeLists.txt")):
            print(f"{dir_path}/CMakeLists.txt found, skipping")
        else:
            generate_cmake(dir_path, files)
