ARG PYTHON_VERSION=3.9
from python:${PYTHON_VERSION}

ARG EXTENSION_VERSION
ARG DUCKDB_VERSION
ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

# Confirm our "required" arguments have values
RUN test -n "$EXTENSION_VERSION" && test -n "$DUCKDB_VERSION"

# Install DuckDB
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Install the Python Helper Library, assumes the wheel has been copied to our cwd
COPY ducktables-${EXTENSION_VERSION}-py3-none-any.whl /tmp
RUN python -m pip install --upgrade pip
RUN pip install  /tmp/ducktables-${EXTENSION_VERSION}-py3-none-any.whl && \
    rm /tmp/ducktables-${EXTENSION_VERSION}-py3-none-any.whl

# Install the UDF Extension, assumes the extension has been copied to our cwd
RUN mkdir -p /root/.duckdb/extensions/v${DUCKDB_VERSION}/${OS_PLATFORM}/
COPY pytables.duckdb_extension /root/.duckdb/extensions/v${DUCKDB_VERSION}/${OS_PLATFORM}/

COPY test-suite.sql /
CMD bash -c 'cat test-suite.sql | duckdb -unsigned'

