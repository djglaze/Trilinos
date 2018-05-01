MACRO(TRIL_SET_BOOL_CACHE_VAR  VAR_NAME  VAR_VALUE)
  IF ("${${VAR_NAME}}" STREQUAL "")
    MESSAGE("-- " "Setting ${VAR_NAME}='${VAR_VALUE}' by default")
    SET(${VAR_NAME} ${VAR_VALUE} CACHE BOOL
      "Set in ${CMAKE_CURRENT_LIST_FILE}")
  ENDIF()
ENDMACRO()