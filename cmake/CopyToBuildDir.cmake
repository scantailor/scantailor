# Reset variables.
FOREACH(conf_ Debug Release MinSizeRel RelWithDebInfo)
	SET(
		"COPY_TO_BUILD_DIR_${conf_}" "" CACHE INTERNAL
		"Files to copy to ${conf_} build directory" FORCE
	)
ENDFOREACH()

# Usage:
# COPY_TO_BUILD_DIR(one or more files [CONFIGURATIONS] conf1 conf2 ...)
MACRO(COPY_TO_BUILD_DIR)
	SET(files_ "")
	SET(confs_ "Debug;Release;MinSizeRel;RelWithDebInfo")
	SET(out_list_ "files_")
	FOREACH(arg_ ${ARGV})
		IF("${arg_}" STREQUAL "CONFIGURATIONS")
			SET(out_list_ "confs_")
			SET(confs_ "")
		ELSE()
			LIST(APPEND ${out_list_} "${arg_}")
		ENDIF()
	ENDFOREACH()
	
	FOREACH(conf_ ${confs_})
		LIST(APPEND "COPY_TO_BUILD_DIR_${conf_}" ${files_})
		
		# Force the new value to be written to the cache.
		SET(
			"COPY_TO_BUILD_DIR_${conf_}" ${COPY_TO_BUILD_DIR_${conf_}}
			CACHE INTERNAL "Files to copy to ${conf_} build directory" FORCE
		)
	ENDFOREACH()
ENDMACRO()


MACRO(GENERATE_COPY_TO_BUILD_DIR_TARGET target_name_)
	SET(script_ "${CMAKE_BINARY_DIR}/copy_to_build_dir.cmake")
	CONFIGURE_FILE("cmake/copy_to_build_dir.cmake.in" "${script_}" @ONLY)
	
	# Copy DLLs and other stuff to ${CMAKE_BINARY_DIR}/<configuration>
	ADD_CUSTOM_TARGET(
		"${target_name_}" ALL
		COMMAND "${CMAKE_COMMAND}" "-DTARGET_DIR=$<TARGET_FILE_DIR:scantailor>"
		"-DCFG=$<CONFIGURATION>" -P "${script_}"
		DEPENDS "${script_}" ${COPY_TO_BUILD_DIR_Debug}
		${COPY_TO_BUILD_DIR_Release} ${COPY_TO_BUILD_DIR_MinSizeRel}
		${COPY_TO_BUILD_DIR_RelWithDebInfo}
    )
ENDMACRO()