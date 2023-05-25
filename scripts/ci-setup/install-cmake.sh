#!/bin/bash

set -e;

wget https://github.com/Kitware/CMake/releases/download/v3.21.3/cmake-3.21.3-linux-x86_64.sh
chmod +x cmake-3.21.3-linux-x86_64.sh
./cmake-3.21.3-linux-x86_64.sh --skip-license --prefix=/usr/local
cmake --version
