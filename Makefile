.PHONY: all clean format debug release duckdb_debug duckdb_release pull update

all: release

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_DIR := $(dir $(MKFILE_PATH))
PYTHON_VERSION := $(if $(PYTHON_VERSION),$(PYTHON_VERSION),3.9)

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
EXTENSION_FLAGS=-DDUCKDB_OOT_EXTENSION_NAMES="python_udf" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_PATH="$(PROJ_DIR)" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_SHOULD_LINK="TRUE" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_INCLUDE_PATH="$(PROJ_DIR)src/include" -DBUILD_HTTPFS_EXTENSION="TRUE"

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
	cp pythonpkgs/ducktables/dist/ducktables-0.1.1-py3-none-any.whl test/extension-integration/
	cp build/release/extension/python_udf/python_udf.duckdb_extension test/extension-integration/
	cd test/extension-integration/ && \
	docker build --build-arg PYTHON_VERSION=$(PYTHON_VERSION) --build-arg EXTENSION_VERSION=0.1.1 --build-arg DUCKDB_VERSION=0.8.0 -t extension-integration-tests . && \
	docker run --rm --interactive extension-integration-tests

# Test the latest release of the extension against a download of DuckDB
post-release-integration:
	if [ -z "$(RELEASE_SHA)" ]; then echo "Please specify a RELEASE_SHA to test against;"; exit 1; fi
	cd test/post-release-integration/ && \
	docker build --build-arg RELEASE_SHA=$(RELEASE_SHA) --build-arg PYTHON_VERSION=3.9 --build-arg EXTENSION_VERSION=0.1.1 --build-arg DUCKDB_VERSION=0.8.0 -t post-release-integration . && docker run --rm --interactive post-release-integration

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
