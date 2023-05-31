#!/bin/bash

set -e;

if [ 1 != $# ]; then
    echo "usage: $0 python-version";
    exit 1;
fi

export PYTHON_VERSION="$1"
# ls -l build/release

# ls -l ~/.ccache/

make release
make test_release
