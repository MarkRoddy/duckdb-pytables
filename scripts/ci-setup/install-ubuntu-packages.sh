#!/bin/bash

set -e;

apt-get update -y -qq
apt-get install -y -qq software-properties-common
# add-apt-repository ppa:git-core/ppa
# apt-get update -y -qq
apt-get install -y -qq ninja-build make gcc-multilib g++-multilib libssl-dev wget zip maven unixodbc-dev libc6-dev-i386 lib32readline6-dev libssl-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip build-essential checkinstall libffi-dev curl libz-dev openssh-client clang-format ccache docker.io wget

# openjdk-8-jdk
