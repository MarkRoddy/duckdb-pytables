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

function exit-error () {
    cat-log
    if [ -n "$1" ]; then
        status "$1";
    fi
    exit 1;
}

function cat-log() {
    if [ -f /tmp/get-pytables.py.log ]; then
        cat /tmp/get-pytables.py.log;
    fi
}

function status () {
    echo -n "Test Status Message: " 1>&2;
    echo "$1" 1>&2;
}

if [ -z "$GITHUB_ACCESS_TOKEN" ]; then
    exit-error "Missing GITHUB_ACCESS_TOKEN needed to run sql script"
fi

status "Install should fail because of missing DuckDB"
python${PYTHON_VERSION} /get-pytables.py | tee output.txt
EC="${PIPESTATUS[0]}";
if [ $EC -eq 0 ]; then
    exit-error "Installer did not exit on missing duckdb"
fi
if grep -nq 'duckdb' output.txt; then
    status "Found reference to missing duckdb as expected";
else
    exit-error "Did not find reference to missing DuckDB in output as expected";
fi

status "Installing DuckDB"
test -n "$DUCKDB_VERSION" 
curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip
status "Installation of DuckDB Complete"

# Installation should now fail because of the missing shared library. Our script will issue
# a warning and then try to install the extension which will fail.
# Note we don't prevent the install as finding libpython is something that is fraught with false
# negatives (detecting it's missing when it is present). So we confirm the warning but don't
# expect the process to fail.
python${PYTHON_VERSION} /get-pytables.py | tee output.txt;
if [ "${PIPESTATUS[0]}" -eq 0 ]; then
    exit-error "Installer script succeeded unexpectedly, should have failed due to missing libpython";
fi
if grep -q libpython output.txt; then
    status "Found reference to libpython as expected";
else
    exit-error "Did not find reference to libpython in output as expected";
fi

status "Installing libpython"
apt-get update -y # containers apt copies may be out of date depending on when it was built
apt-get install -y -qq libpython${PYTHON_VERSION}

status "Installation should now work but our warning is still present as script is unable to find it"
python${PYTHON_VERSION} /get-pytables.py | tee output.txt
if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    exit-error "Installer script unexpectedly exitted with an error";
fi
if grep -q libpython output.txt; then
    status "Found reference to libpython as expected";
else
    # We use the DeadSnakes PPA for installing various versions of python, and it does
    # not update ldconfig cache. However, standard Ubuntu packages do. So we have an
    # exception to our expectation that the cache won't be updated which we need to
    # check for.
    if lsb_release -a | grep "Ubuntu 20.04"; then
        if [ "3.8" == "$PYTHON_VERSION" ]; then
           status "Running stock Ubuntu Python, ignore lack of a warning";
        else
            exit-error "Did not find reference to libpython in output as expected";
        fi
    else
        exit-error "Did not find reference to libpython in output as expected";
    fi
fi

status "Updating the shared library cache so our script will be able to find it"
ldconfig

status "Now that we've installed the library + updated ldconfig cache, we should not have a warning."
python${PYTHON_VERSION} /get-pytables.py | tee output.txt
if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    exit-error "Installer script unexpectedly exitted with an error";
fi
if grep -q libpython output.txt; then
    exit-error "Found reference to libpython that we did not expect because we've updated ldconfig cache";
else
    status "No reference to libpython as expected"
fi

status "Extension installation should work now, but the Python package install will not."
python${PYTHON_VERSION} /get-pytables.py
if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    exit-error "Installer script unexpectedly exitted with an error";
fi

status "Confirming output regarding pip being missing"
if grep -q "No 'pip' module found." output.txt; then
    status "Found expected missing pip error message";
else
    exit-error "Did not find expected missing pip message";
fi
python${PYTHON_VERSION} -c "import ducktables" | tee output.txt
EC="${PIPESTATUS[0]}";
if [ $EC -eq 0 ]; then
    status "We were able to install pypackage but don't expect to be able to."
    status "This is most likely a bug in the test setup where we are some how"
    status "not preventing the package install as expected/desired"
    exit-error
fi

# Even though we were not able to install the Python package, the extension should
# be installed. Confirm this is the case.
cat test-load-extension.sql | duckdb -unsigned | tee output.txt
EC="${PIPESTATUS[1]}";
if [ $EC -ne 0 ]; then
    exit-error "DuckDB could not load the extension as expected"
fi



# Get PIP working so we can test our package install
apt-get install -y -qq python${PYTHON_VERSION}-distutils && \
    curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION}

# Re-run the installer, python package should be installed this time.
python${PYTHON_VERSION} /get-pytables.py && python${PYTHON_VERSION} -c "import ducktables"

# Confirm by trying to query a ducktables function
# cat /test-run-query.sql | duckdb -unsigned | tee output.txt
cat /test-run-query.sql | duckdb -unsigned | tee output.txt
if [ "${PIPESTATUS[1]}" -ne 0 ]; then
    exit-error "DuckDB unexpectedly failed to run the query";
fi
if grep -q "duckdb-wasm" output.txt; then
    status "Found expected reference to duckdb-wasm (github repo that should be returned)";
else
    exit-error "Missing expected reference to duckdb-wasm, github repo";
fi

cat-log
