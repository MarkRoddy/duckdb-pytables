.PHONY: all clean format debug release duckdb_debug duckdb_release pull update


OBSV_TAG := $(shell git submodule status|cut -d '(' -f 2|cut -d ')' -f 1)
DDB_TAG := $(shell basename `pwd`|sed 's/^duckdb-python-udf-\(.*\)-[a-z]*/\1/')
ifeq (${DDB_TAG}, "")
	DDB_TAG=${OBSV_TAG}
endif
BUILD_FLAVOR=$(shell basename `pwd`|sed 's/^duckdb-python-udf-.*-\([a-z]*\)/\1/')
ifeq (${BUILD_FLAVOR}, "")
	BUILD_FLAVOR=release
endif

all: submod_check ${BUILD_FLAVOR}

submod_check: duckdb
	@if [ "${DDB_TAG}" != "${OBSV_TAG}" ]; then \
		echo "Your submodule checkout (${OBSV_TAG}) does not match your directory name (${DDB_TAG})" && exit 1; \
	else \
		echo "Found submodule at ${OBSV_TAG} as expected"; \
	fi

submod: pull
	cd duckdb && git co ${DDB_TAG}

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJ_DIR := $(dir $(MKFILE_PATH))

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
EXTENSION_FLAGS=-DDUCKDB_OOT_EXTENSION_NAMES="python_udf" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_PATH="$(PROJ_DIR)" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_SHOULD_LINK="TRUE" -DDUCKDB_OOT_EXTENSION_PYTHON_UDF_INCLUDE_PATH="$(PROJ_DIR)src/include"

pull:
	git submodule init
	git submodule update --recursive --remote

clean:
	rm -rf build
	rm -rf testext
	cd duckdb && make clean

# Main build
build: ${BUILD_FLAVOR}

debug:
	mkdir -p  build/debug && \
	cmake $(GENERATOR) $(FORCE_COLOR) $(EXTENSION_FLAGS) ${CLIENT_FLAGS} -DENABLE_SANITIZER=TRUE -DFORCE_ASSERT=TRUE -DEXTENSION_STATIC_BUILD=1 -DCMAKE_BUILD_TYPE=Debug ${BUILD_FLAGS} -S ./duckdb/ -B build/debug && \
	cmake --build build/debug --config Debug

release:
	mkdir -p build/release && \
	cmake $(GENERATOR) $(FORCE_COLOR) $(EXTENSION_FLAGS) ${CLIENT_FLAGS} -DEXTENSION_STATIC_BUILD=1 -DCMAKE_BUILD_TYPE=Release ${BUILD_FLAGS} -S ./duckdb/ -B build/release && \
	cmake --build build/release --config Release

extension: extension-${BUILD_FLAVOR}

extension-release:
	cmake --build build/release --config Release

extension-debug:
	cmake --build build/debug --config Debug

# Main tests
test: test_${BUILD_FLAVOR}

test_release: release
	python3 udfs.py
	./build/release/test/unittest --test-dir . "[sql]"

test_debug: debug
	python3 udfs.py
	ASAN_OPTIONS=detect_leaks=1 ./build/debug/test/unittest --test-dir . "[sql]"

check-format:
	find src/ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -Werror --sort-includes=0 -style=file --dry-run

format:
	find src/ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -Werror --sort-includes=0 -style=file -i

update:
	git submodule update --remote --merge
