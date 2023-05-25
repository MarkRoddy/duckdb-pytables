#!/bin/bash

set -e;

wget https://github.com/git/git/archive/refs/tags/v2.18.5.tar.gz
tar xvf v2.18.5.tar.gz
cd git-2.18.5
make
make prefix=/usr install
git --version
git config --global --add safe.directory '*'
