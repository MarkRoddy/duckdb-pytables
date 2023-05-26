
from ubuntu:20.04

ARG PYTHON_VERSION
ARG EXTENSION_VERSION
ARG DUCKDB_VERSION
ARG RELEASE_SHA
ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

RUN apt-get update -y && \
    apt-get install -y -qq software-properties-common && \
    apt-get update -y
RUN apt-get install -y -qq gettext-base curl unzip python${PYTHON_VERSION} libpython${PYTHON_VERSION} python${PYTHON_VERSION}-distutils

# Confirm our "required" arguments have values
RUN test -n "$EXTENSION_VERSION" && test -n "$DUCKDB_VERSION" && test -n "$PYTHON_VERSION" && test -n "$RELEASE_SHA"

# Install DuckDB
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Install the Python Helper Library, assumes the wheel has been copied to our cwd
RUN curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION} && \
    python${PYTHON_VERSION} -m pip install --upgrade pip && \
    pip${PYTHON_VERSION} install  ducktables==${EXTENSION_VERSION}


env DUCKDB_VERSION=${DUCKDB_VERSION}
env PYTHON_VERSION=${PYTHON_VERSION}
env RELEASE_SHA=${RELEASE_SHA}
COPY test-suite.sql.template /
RUN cat test-suite.sql.template | envsubst > /test-suite.sql
CMD bash -c 'cat test-suite.sql | duckdb -unsigned'

