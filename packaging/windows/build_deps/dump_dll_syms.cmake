# We are operating on files from the current directory.
FILE(GLOB DLL_FILES "*.dll" "*.DLL")
FOREACH(dll_file ${DLL_FILES})
	MESSAGE(STATUS "Dumping symbols from ${dll_file}")
	EXECUTE_PROCESS(
		COMMAND "${DUMP_SYMS_EXECUTABLE}" "${dll_file}"
		OUTPUT_FILE "${SYMBOLS_PATH}/temp.sym" 
	)
	EXECUTE_PROCESS(
		COMMAND "${CMAKE_COMMAND}" "-DSYMBOLS_PATH=${SYMBOLS_PATH}"
		-P "${MOVE_SYMBOLS_SCRIPT}"
	)
ENDFOREACH()
