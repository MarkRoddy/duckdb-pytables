
.PHONY: all clean format debug release duckdb_debug duckdb_release pull update

all: release

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_DIR := $(dir $(MKFILE_PATH))
PYTHON_VERSION := $(if $(PYTHON_VERSION),$(PYTHON_VERSION),3.9)
EXTENSION_VERSION := $(shell cat pythonpkgs/ducktables/version.txt)
DUCKDB_VERSION := $(if $(DUCKDB_VERSION),$(DUCKDB_VERSION),0.8.0)

OSX_BUILD_UNIVERSAL_FLAG=
ifeq (${OSX_BUILD_UNIVERSAL}, 1)
	OSX_BUILD_UNIVERSAL_FLAG=-DOSX_BUILD_UNIVERSAL=1
endif
ifeq (${STATIC_LIBCPP}, 1)
	STATIC_LIBCPP=-DSTATIC_LIBCPP=TRUE
endif

ifeq ($(GEN),ninja)
	GENERATOR=-G "Ninja"
	FORCE_COLOR=-DFORCE_COLORED_OUTPUT=1
endif

BUILD_FLAGS=-DEXTENSION_STATIC_BUILD=1 -DBUILD_TPCH_EXTENSION=0 -DBUILD_PARQUET_EXTENSION=0 ${OSX_BUILD_UNIVERSAL_FLAG} ${STATIC_LIBCPP}

CLIENT_FLAGS :=

# These flags will make DuckDB build the extension
EXTENSION_FLAGS=-DDUCKDB_OOT_EXTENSION_NAMES="pytables" -DDUCKDB_OOT_EXTENSION_PYTABLES_PATH="$(PROJ_DIR)" -DDUCKDB_OOT_EXTENSION_PYTABLES_SHOULD_LINK="TRUE" -DDUCKDB_OOT_EXTENSION_PYTABLES_INCLUDE_PATH="$(PROJ_DIR)src/include" -DBUILD_HTTPFS_EXTENSION="TRUE"

pull:
	git submodule init
	git submodule update --recursive --remote

revert-submodules:
	git submodule deinit -f .
	git submodule update --init

clean:
	rm -rf build
	rm -rf testext
	cd duckdb && make clean

# Main build
debug:
	mkdir -p  build/debug && \
	cmake $(GENERATOR) $(FORCE_COLOR) $(EXTENSION_FLAGS) ${CLIENT_FLAGS} -DENABLE_SANITIZER=TRUE -DFORCE_ASSERT=TRUE -DEXTENSION_STATIC_BUILD=1 -DCMAKE_BUILD_TYPE=Debug ${BUILD_FLAGS} -S ./duckdb/ -B build/debug && \
	cmake --build build/debug --config Debug

release:
	mkdir -p build/release && \
	cmake $(GENERATOR) $(FORCE_COLOR) $(EXTENSION_FLAGS) ${CLIENT_FLAGS} -DPYTHON_VERSION=$(PYTHON_VERSION) -DEXTENSION_STATIC_BUILD=1 -DCMAKE_BUILD_TYPE=Release ${BUILD_FLAGS} -S ./duckdb/ -B build/release && \
	cmake --build build/release --config Release

extension-release:
	cmake --build build/release --config Release

extension-debug:
	cmake --build build/debug --config Debug

python-ci: ./scripts/python-ci.sh
	bash ./scripts/python-ci.sh

python-release:
	rm -f pythonpkgs/ducktables/dist/duck*
	bash ./scripts/python-release.sh

python-test-integration:
	bash ./scripts/python-test-integration.sh

# Tests a build of the extension against a download of DuckDB
extension-integration-tests:
	cp pythonpkgs/ducktables/dist/ducktables-$(EXTENSION_VERSION)-py3-none-any.whl test/extension-integration/
	cp build/release/extension/pytables/pytables.duckdb_extension test/extension-integration/
	cd test/extension-integration/ && \
	docker build \
	  --build-arg PYTHON_VERSION=$(PYTHON_VERSION) \
	  --build-arg EXTENSION_VERSION=$(EXTENSION_VERSION) \
	  --build-arg DUCKDB_VERSION=$(DUCKDB_VERSION) \
	  --build-arg GITHUB_ACCESS_TOKEN=$(GITHUB_ACCESS_TOKEN) \
	  -t extension-integration-tests . && \
	docker run --rm --interactive extension-integration-tests

# Test the latest release of the extension against a download of DuckDB
post-release-integration:
	if [ -z "$(RELEASE_SHA)" ]; then echo "Please specify a RELEASE_SHA to test against;"; exit 1; fi
	cd test/post-release-integration/ && \
	docker build --build-arg RELEASE_SHA=$(RELEASE_SHA) --build-arg PYTHON_VERSION=$(PYTHON_VERSION) --build-arg EXTENSION_VERSION=$(EXTENSION_VERSION) --build-arg DUCKDB_VERSION=$(DUCKDB_VERSION) -t post-release-integration . && docker run --rm --interactive post-release-integration


# Builds a container image with the specified version of Python suitable
# for doing local development without having to install dependencies
build-container-devel:
	if [ -z "$(PYTHON_VERSION)" ]; then echo "Please specify a PYTHON_VERSION"; exit 1; fi
	docker build \
	  --build-arg PYTHON_VERSION=$(PYTHON_VERSION) \
	  --build-arg UID=$(shell id -u) \
	  --build-arg GID=$(shell id -g) \
	  --build-arg USER=$(USER) \
	  -t build-duckdb-python-py$(PYTHON_VERSION) .

# Assuming you've built the image above, runs a Docker container with your local source
# checkout mounted and compiles the project.
container-compile:
	docker run --rm --interactive \
	  --volume "$(HOME)/.ccache/:/home/$(USER)/.ccache" \
	  --volume "$(shell pwd)/:$(shell pwd)" \
	  build-duckdb-python-py$(PYTHON_VERSION) \
	  bash -c "cd $(shell pwd) && bash scripts/docker-build-in-container.sh $(PYTHON_VERSION)"

# Executes the installer script and check that it correctly detects some common misconfigurations
test-installer:
	cd installer/ && \
	docker build \
	  --build-arg PYTHON_VERSION=$(PYTHON_VERSION) \
	  --build-arg DUCKDB_VERSION=$(DUCKDB_VERSION) \
	  --build-arg GITHUB_ACCESS_TOKEN=$(GITHUB_ACCESS_TOKEN) \
	  -t installer-tests-ddb$(DUCKDB_VERSION)-py$(PYTHON_VERSION) . 

shell-installer:
	docker run --rm -it installer-tests-ddb$(DUCKDB_VERSION)-py$(PYTHON_VERSION) bash

# Main tests
test: test_release

test_legacy: test_legacy_release

test_legacy_release:
	python3 udfs.py
	./build/release/test/unittest --test-dir . "[legacy]"

test_legacy_debug:
	python3 udfs.py
	./build/debug/test/unittest --test-dir . "[legacy]"

test_release:
	python3 udfs.py
	PYTHONPATH=. ./build/release/test/unittest --test-dir . "[sql]"

test_debug:
	python3 udfs.py
	PYTHONPATH=. ASAN_OPTIONS=detect_leaks=1 ./build/debug/test/unittest --test-dir . "[sql]"

check-format:
	find src/ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -Werror --sort-includes=0 -style=file --dry-run

format:
	find src/ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -Werror --sort-includes=0 -style=file -i

fmt: format

update:
	git submodule update --remote --merge
