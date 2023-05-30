cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME python_udf)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})

set(PYTHON_VERSION "3.8" CACHE STRING "Desired Python version")
find_package(Python ${PYTHON_VERSION} EXACT COMPONENTS Development)
get_filename_component(PYTHON_LIB_NAME ${Python_LIBRARIES} NAME)
string(REPLACE "." "" Python_VERSION_NO_DOTS ${Python_VERSION})

configure_file(src/include/config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(src/include ${Python_INCLUDE_DIRS})

FILE(GLOB EXTENSION_SOURCES src/*.cpp)

# Define Py_LIMITED_API for all source files, pins us to Python 3.4.
add_definitions(-DPy_LIMITED_API=0x03040000)

add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

target_link_libraries(${EXTENSION_NAME} ${Python_LIBRARIES})

target_compile_definitions(${EXTENSION_NAME} PRIVATE PYTHON_VERSION=${Python_VERSION_NO_DOTS})
install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
