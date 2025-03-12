add_subdirectory(parser)
add_subdirectory(synthesis)

include_directories(${PARSER_INCLUDE_PATH} ${SYNTHESIS_INCLUDE_PATH} ${EXT_INCLUDE_PATH})
add_executable(syn Main.cpp)

target_link_libraries(syn ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})

INSTALL (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
