cmake_minimum_required(VERSION 2.8.12)

# Set extension name here
set(TARGET_NAME python_udf)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
project(${TARGET_NAME})
include_directories(src/include /usr/include/python3.9)

set(EXTENSION_SOURCES src/python_udf_extension.cpp)
add_library(${EXTENSION_NAME} STATIC ${EXTENSION_SOURCES})

set(PARAMETERS "-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${EXTENSION_SOURCES})

# set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
# target_link_libraries(${TARGET_NAME} duckdb_static ${DUCKDB_EXTRA_LINK_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,ALL)
# set_target_properties(${EXTENSION_NAME} PROPERTIES LINK_FLAGS "-I/usr/include/python3.9 -lpython3.9" )
# target_link_libraries(LibsModule -lpthread)
target_link_libraries(${EXTENSION_NAME} -lpython3.9)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
