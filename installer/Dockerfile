FROM ubuntu:20.04 AS minimal

ARG OS=linux
ARG PLATFORM=amd64
ARG OS_PLATFORM=${OS}_${PLATFORM}

RUN apt-get update -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq keyboard-configuration && \
    apt-get install -y -qq gettext-base curl unzip software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update -y

# Just enough of a Python install to run our pytables-install script and detect that it handles error conditions
ARG PYTHON_VERSION
RUN test -n "$PYTHON_VERSION"
RUN apt-get install -y -qq python${PYTHON_VERSION}

# Confirm we have our expected version of python installed
RUN python$PYTHON_VERSION --version


COPY pytables-install.py /tmp/

# This should fail because of a missing libpython
COPY test-01-expected-failure.sh /
RUN bash /test-01-expected-failure.sh ${PYTHON_VERSION}

# Install the libpython
RUN apt-get install -y -qq libpython${PYTHON_VERSION}


# This should fail because of missing DuckDB
RUN bash /test-01-expected-failure.sh ${PYTHON_VERSION}

# Install DuckDB
ARG DUCKDB_VERSION
RUN test -n "$DUCKDB_VERSION" 
RUN curl -Lo /tmp/duckdb_cli.zip "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/duckdb_cli-${OS}-${PLATFORM}.zip" && \
    unzip -d /tmp/ /tmp/duckdb_cli.zip  && \
    mv /tmp/duckdb /usr/bin/duckdb && \
    rm /tmp/duckdb_cli.zip

# Installation should work now that we have requirements satisfied, though no package installed, so our test query will fail
RUN python${PYTHON_VERSION} /tmp/pytables-install.py
COPY test-suite.sql /
# todo: verify that this failed
RUN cat /test-suite.sql | duckdb -unsigned || true


# Install pip, and then the package should be installed
RUN apt-get install -y -qq python${PYTHON_VERSION}-distutils && \
    curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION}
# RUN python${PYTHON_VERSION} -m ensurepip --upgrade
RUN python${PYTHON_VERSION} /tmp/pytables-install.py && python${PYTHON_VERSION} -c "import ducktables"

# Confirm by trying to query a ducktables function
RUN cat /test-suite.sql | duckdb -unsigned || true

