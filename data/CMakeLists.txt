file(GLOB_RECURSE COPY_FILES
                  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                  assets/*)

foreach(FILENAME ${COPY_FILES})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    set(BIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILENAME}")

    add_custom_command(
        OUTPUT ${BIN}
        DEPENDS ${SRC}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${BIN}
    )
endforeach(FILENAME)

add_custom_target(
  export-asset-files ALL
  DEPENDS ${COPY_FILES}
  COMMENT "Exporting files into build tree"
)
