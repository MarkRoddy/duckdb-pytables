#!/bin/bash

set -e;

bash scripts/ci-setup/install-ubuntu-packages.sh
bash scripts/ci-setup/install-git-src.sh
bash scripts/ci-setup/install-cmake.sh
bash scripts/ci-setup/install-openssl.sh

# version-check
ldd --version ldd
git --version
git log -1 --format=%h


# Make sure subsequent steps can find our OpenSSL installation
# - name: Set environment
# run: echo "LD_LIBRARY_PATH=/usr/local/ssl/lib:$LD_LIBRARY_PATH" >> $GITHUB_ENV
export LD_LIBRARY_PATH=/usr/local/ssl/lib:$LD_LIBRARY_PATH;

export GEN=ninja
export STATIC_LIBCPP=1
make release

