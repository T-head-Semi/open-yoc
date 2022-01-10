file(GLOB_RECURSE LIBSOURCE "${COMPONENTS_DIR}/ulog/src/ulog_linux.c")

include_directories(${COMPONENTS_DIR}/ulog/include)

add_library(ulog STATIC ${LIBSOURCE})