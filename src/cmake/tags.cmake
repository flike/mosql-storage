IF(UNIX)
ADD_CUSTOM_TARGET (ctags
COMMAND ctags -R -f CTAGS
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
ENDIF()
