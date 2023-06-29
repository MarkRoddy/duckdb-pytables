#!/bin/bash

set -e;

# Not to be confused with `scripts/extension-upload.sh` which is used by this script. That
# script is basically just for translating the S3 path from arguments. This contains all
# logic for the step that Github Actions performs. Feel free to merge the two if you ever
# feel like it.

git config --global --add safe.directory '*'
cd duckdb
git fetch --tags
export DUCKDB_VERSION=`git tag --points-at HEAD`
export DUCKDB_VERSION=${DUCKDB_VERSION:=`git log -1 --format=%h`}
cd ..
if [[ "$AWS_ACCESS_KEY_ID" == "" ]] ; then
    echo 'No key set, skipping'
elif [[ "$GITHUB_REF" =~ ^(refs/tags/v.+)$ ]] ; then
    echo "Uploading release tagged $GITHUB_REF";
    python3 -m pip install pip awscli
    ./scripts/extension-upload.sh pytables ${{ github.ref_name }} $DUCKDB_VERSION ${{matrix.arch}} $BUCKET_NAME true python${{ matrix.python_version }}
elif [[ "$GITHUB_REF" =~ ^(refs/heads/main)$ ]] ; then
    echo "Uploading tip of main branch $GITHUB_REF";
    python3 -m pip install pip awscli
    ./scripts/extension-upload.sh pytables `git log -1 --format=%h` $DUCKDB_VERSION ${{matrix.arch}} $BUCKET_NAME false python${{ matrix.python_version }}
else
    echo "Build is for non tag on a branch other than main, skipping"
fi
