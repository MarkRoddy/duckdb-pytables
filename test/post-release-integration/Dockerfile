
from ubuntu:20.04

ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

ARG PYTHON_VERSION
RUN test -n "$PYTHON_VERSION"
RUN apt-get update -y && \
    apt-get install -y -qq software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update -y
RUN apt-get install -y -qq gettext-base curl unzip python${PYTHON_VERSION} libpython${PYTHON_VERSION} python${PYTHON_VERSION}-distutils python${PYTHON_VERSION}-venv

# If we're using a deadsnakes Python, we need to manually update ld cache
# so our installer script will find it.
RUN ldconfig

# Confirm we have our expected version of python installed
RUN python$PYTHON_VERSION --version

# Ensure we have a functioning version of Pip
RUN curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION} && \
    python${PYTHON_VERSION} -m pip install --upgrade pip

# Install DuckDB
ARG DUCKDB_VERSION
RUN test -n "$DUCKDB_VERSION" 
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Use the installer script to pull down the extension + the Python library
ARG RELEASE_SHA
RUN test -n "$RELEASE_SHA"
RUN curl -L -o /tmp/get-pytables.py https://github.com/MarkRoddy/duckdb-pytables/releases/download/latest/get-pytables.py && \
    python$PYTHON_VERSION /tmp/get-pytables.py --ext-version "$RELEASE_SHA"

env DUCKDB_VERSION=${DUCKDB_VERSION}
env PYTHON_VERSION=${PYTHON_VERSION}
COPY test-query.sql /
CMD bash -c 'cat test-query.sql | duckdb -unsigned'

