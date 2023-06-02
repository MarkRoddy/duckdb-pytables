#!/bin/bash

set -e;

if [ 4 != $# ]; then
    echo "Args: $@"
    echo "usage: $0 os platform python-version duckdb-version";
    exit 1;
fi
OS="$1"
PLATFORM="$2"
PYTHON_VERSION="$3";
DUCKDB_VERSION="$4";

# This should fail because of a missing libpython
python${PYTHON_VERSION} /pytables-install.py | tee output.txt
EC="${PIPESTATUS[0]}";
if [ 0 -eq "$EC" ]; then
    echo "Installer did not exit on missing libpython was missing as expected"
    exit 1
elif ! grep -q libpython output.txt; then
    echo "Did not find reference to libpython in output as expected";
fi

# Install the libpython
apt-get install -y -qq libpython${PYTHON_VERSION}


# This should fail because of missing DuckDB
python${PYTHON_VERSION} /pytables-install.py | tee output.txt
EC="${PIPESTATUS[0]}";
if [ $EC -eq 0 ]; then
    echo "Installer did not exit on missing duckdb"
    exit 1
elif ! grep -q 'duckdb.*PATH' output.txt; then
    echo "Did not find reference to missing DuckDB in output as expected";
fi



test -n "$DUCKDB_VERSION" 
curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Extension installation should work now, but the Python package install will not.
python${PYTHON_VERSION} /pytables-install.py
cat test-load-extension.sql | duckdb -unsigned | tee output.txt
EC="${PIPESTATUS[1]}";
if [ $EC -nq 0 ]; then
    echo "DuckDB did not find the extension as expected"
    exit 1
fi
python${PYTHON_VERSION} -c "import ducktables" | tee output.txt
EC="${PIPESTATUS[0]}";
if [ $EC -eq 0 ]; then
    echo "We were able to install pypackage but don't expect to be able to."
    echo "This is most likely a bug in the test setup where we are some how"
    echo "not preventing the package install as expected/desired"
    exit 1
fi



# Get PIP working so we can test our package install
apt-get install -y -qq python${PYTHON_VERSION}-distutils && \
    curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION}

# Re-run the installer, python package should be installed this time.
python${PYTHON_VERSION} /tmp/pytables-install.py && python${PYTHON_VERSION} -c "import ducktables"

# Confirm by trying to query a ducktables function
RUN cat /test-query.sql | duckdb -unsigned 

