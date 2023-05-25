#!/bin/bash

set -e;

wget https://www.openssl.org/source/openssl-1.1.1c.tar.gz
tar -xf openssl-1.1.1c.tar.gz
cd openssl-1.1.1c
./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl no-shared zlib-dynamic
make
make install
echo "/usr/local/ssl/lib" > /etc/ld.so.conf.d/openssl-1.1.1c.conf
ldconfig -v
