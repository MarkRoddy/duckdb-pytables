from python:3.10

RUN mkdir -p /duckdb-python-udf/

WORKDIR /duckdb-python-udf/
COPY scripts /duckdb-python-udf/scripts

RUN pwd && ls -l  && \
    bash scripts/ci-setup/install-ubuntu-packages.sh  && \
    bash scripts/ci-setup/install-git-src.sh && \
    bash scripts/ci-setup/install-cmake.sh && \
    bash scripts/ci-setup/install-openssl.sh 


COPY Makefile CMakeLists.txt udfs.py /duckdb-python-udf/
COPY duckdb /duckdb-python-udf/duckdb
COPY src /duckdb-python-udf/src
COPY test /duckdb-python-udf/test
# COPY .git /duckdb-python-udf/.git



# version-check
RUN ldd --version ldd && \
    git --version && \
    # git log -1 --format=%h


# Make sure subsequent steps can find our OpenSSL installation
# - name: Set environment
# run: echo "LD_LIBRARY_PATH=/usr/local/ssl/lib:$LD_LIBRARY_PATH" >> $GITHUB_ENV
# export LD_LIBRARY_PATH=/usr/local/ssl/lib:$LD_LIBRARY_PATH;

env GEN=ninja
env STATIC_LIBCPP=1
RUN ls -l /duckdb-python-udf/
RUN ls -l /duckdb-python-udf/duckdb/
RUN ls -l /duckdb-python-udf/src/
RUN make release

