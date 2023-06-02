
from ubuntu:20.04

ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

ARG PYTHON_VERSION
RUN test -n "$PYTHON_VERSION"
RUN apt-get update -y && \
    apt-get install -y -qq software-properties-common && \
    apt-get update -y
RUN apt-get install -y -qq gettext-base curl unzip python${PYTHON_VERSION} libpython${PYTHON_VERSION} python${PYTHON_VERSION}-distutils python${PYTHON_VERSION}-venv

# Confirm we have our expected version of python installed
RUN python$PYTHON_VERSION --version

# Install DuckDB
ARG DUCKDB_VERSION
RUN test -n "$DUCKDB_VERSION" 
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Install the Python Helper Library, assumes the wheel has been copied to our cwd
ARG EXTENSION_VERSION
RUN test -n "$EXTENSION_VERSION" 
RUN curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION} && \
    python${PYTHON_VERSION} -m pip install --upgrade pip && \
    python${PYTHON_VERSION} -m pip install ducktables==${EXTENSION_VERSION} && \
    python${PYTHON_VERSION} -c "import ducktables"

ARG RELEASE_SHA
RUN test -n "$RELEASE_SHA"
env DUCKDB_VERSION=${DUCKDB_VERSION}
env PYTHON_VERSION=${PYTHON_VERSION}
env RELEASE_SHA=${RELEASE_SHA}
COPY test-suite.sql.template /
RUN cat test-suite.sql.template | envsubst > /test-suite.sql
CMD bash -c 'cat test-suite.sql | duckdb -unsigned'

