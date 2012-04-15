# Reset variables.
FOREACH(conf_ Debug Release MinSizeRel RelWithDebInfo)
	SET(
		"COPY_TO_BUILD_DIR_${conf_}" "" CACHE INTERNAL
		"Files to copy to ${conf_} build directory" FORCE
	)
ENDFOREACH()

# Usage:
# COPY_TO_BUILD_DIR(one or more files [SUBDIR subdir] [CONFIGURATIONS] conf1 conf2 ...)
MACRO(COPY_TO_BUILD_DIR)
	SET(files_ "")
	SET(confs_ "Debug;Release;MinSizeRel;RelWithDebInfo")
	SET(subdir_ "")
	SET(out_list_ "files_")
	FOREACH(arg_ ${ARGV})
		IF("${arg_}" STREQUAL "SUBDIR")
			SET(out_list_ "subdir_")
		ELSEIF("${arg_}" STREQUAL "CONFIGURATIONS")
			SET(out_list_ "confs_")
			SET(confs_ "")
		ELSE()
			LIST(APPEND ${out_list_} "${arg_}")
		ENDIF()
	ENDFOREACH()
	
	LIST(LENGTH subdir_ num_subdirs_)
	IF("${num_subdirs_}" GREATER 1)
		MESSAGE(FATAL_ERROR "Multiple sub-directories aren't allowed!")
	ENDIF()
	
	FOREACH(conf_ ${confs_})
		FOREACH(file_ ${files_})
			IF("${subdir_}" STREQUAL "")
				LIST(APPEND "COPY_TO_BUILD_DIR_${conf_}" "${file_}")
			ELSE()
				LIST(APPEND "COPY_TO_BUILD_DIR_${conf_}" "${file_}=>${subdir_}")
			ENDIF()
		ENDFOREACH()
		
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
	
	SET(
		src_files_
		${COPY_TO_BUILD_DIR_Debug} ${COPY_TO_BUILD_DIR_Release}
		${COPY_TO_BUILD_DIR_MinSizeRel} ${COPY_TO_BUILD_DIR_RelWithDebInfo}
	)
	SET(deps_ "")
	FOREACH(src_file_ ${src_files_})
		STRING(REGEX REPLACE "(.*)=>.*" "\\1" src_file_ "${src_file_}")
		LIST(APPEND deps_ "${src_file_}")
	ENDFOREACH()
	
	# Copy DLLs and other stuff to ${CMAKE_BINARY_DIR}/<configuration>
	ADD_CUSTOM_TARGET(
		"${target_name_}" ALL
		COMMAND "${CMAKE_COMMAND}" "-DTARGET_DIR=$<TARGET_FILE_DIR:scantailor>"
		"-DCFG=$<CONFIGURATION>" -P "${script_}"
		DEPENDS "${script_}" ${deps_}
    )
ENDMACRO()