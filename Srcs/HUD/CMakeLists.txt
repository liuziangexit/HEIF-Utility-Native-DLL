include_directories(${PROJECT_SOURCE_DIR}/Srcs/common)
include_directories(${PROJECT_SOURCE_DIR}/Srcs/jsonlib)
include_directories(${PROJECT_SOURCE_DIR}/Srcs/parser)
include_directories(${PROJECT_SOURCE_DIR}/Srcs/reader)
include_directories(${PROJECT_SOURCE_DIR}/Srcs/writer)
include_directories(${PROJECT_SOURCE_DIR}/Srcs/reader)
include_directories("${PROJECT_BINARY_DIR}")

set(WRITERAPP_SRCS
	main.cpp)

add_library(${WRITER_EXE} SHARED ${WRITERAPP_SRCS})
target_link_libraries(${WRITER_EXE} json heifwriter)