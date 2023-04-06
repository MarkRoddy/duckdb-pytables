cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME python_udf)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})

find_package(Python REQUIRED COMPONENTS Development)
include_directories(src/include ${Python_INCLUDE_DIRS})


set(EXTENSION_SOURCES src/python_udf_extension.cpp src/pyscalar.cpp src/pytable.cpp src/python_function.cpp)
add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

target_link_libraries(${EXTENSION_NAME} ${Python_LIBRARIES})

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
