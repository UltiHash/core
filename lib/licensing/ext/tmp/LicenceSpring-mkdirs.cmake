# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring-build"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/tmp"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring-stamp"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src"
  "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/benjamin-elias/CLionProjects/core/lib/licensing/ext/src/LicenceSpring-stamp${cfgdir}") # cfgdir has leading slash
endif()
