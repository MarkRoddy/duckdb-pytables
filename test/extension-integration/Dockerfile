FROM ubuntu:20.04

RUN apt-get update -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq keyboard-configuration && \
    apt-get install -y -qq gettext-base curl unzip software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update -y


ARG PYTHON_VERSION
RUN test -n "$PYTHON_VERSION"
RUN apt-get install -y -qq python${PYTHON_VERSION} libpython${PYTHON_VERSION} python${PYTHON_VERSION}-distutils && \
    curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION} && \
    python$PYTHON_VERSION -m pip install --upgrade pip


# Confirm we have our expected version of python installed
RUN python$PYTHON_VERSION --version

ARG EXTENSION_VERSION
ARG DUCKDB_VERSION
ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

# Confirm our "required" arguments have values
RUN test -n "$EXTENSION_VERSION" && test -n "$DUCKDB_VERSION" && \
    echo "Extension: $EXTENSION_VERSION" && \
    echo "DuckDB: $DUCKDB_VERSION"

# Install DuckDB
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip && \
    duckdb --version

# Install the Python Helper Library, assumes the wheel has been copied to our cwd
COPY ducktables-${EXTENSION_VERSION}-py3-none-any.whl /tmp
RUN python$PYTHON_VERSION -m pip install  /tmp/ducktables-${EXTENSION_VERSION}-py3-none-any.whl && \
    rm /tmp/ducktables-${EXTENSION_VERSION}-py3-none-any.whl

# Install the UDF Extension, assumes the extension has been copied to our cwd
RUN mkdir -p /root/.duckdb/extensions/v${DUCKDB_VERSION}/${OS_PLATFORM}/
COPY pytables.duckdb_extension /root/.duckdb/extensions/v${DUCKDB_VERSION}/${OS_PLATFORM}/

ARG GITHUB_ACCESS_TOKEN
ENV GITHUB_ACCESS_TOKEN=${GITHUB_ACCESS_TOKEN}
COPY test-suite.sql /
CMD bash -c 'cat test-suite.sql | duckdb -unsigned'

