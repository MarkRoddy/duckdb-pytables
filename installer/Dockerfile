FROM ubuntu:20.04

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


COPY get-pytables.py /
COPY test-installer.sh /
COPY test-load-extension.sql /
COPY test-run-query.sql /

ARG DUCKDB_VERSION
ENV DUCKDB_VERSION=${DUCKDB_VERSION}
ENV OS=${OS}
ENV PLATFORM=${PLATFORM}
ENV PYTHON_VERSION=${PYTHON_VERSION}
CMD bash /test-installer.sh ${OS} ${PLATFORM} ${PYTHON_VERSION} ${DUCKDB_VERSION}
