# TRANSLATION_SOURCES(<target> <source files>)
#
# Schedules the provided source files to be parsed for translatable strings.
# The target parameter identifies a translation set.  Usually it corresponds
# to an executable or a library target, though it doesn't have to.
# This macro may be called multiple times, possibly from different directories.
# To be followed by UPDATE_TRANSLATION_SET()
#
MACRO(TRANSLATION_SOURCES _target)
	FILE(GLOB _sources ABSOLUTE ${ARGN})
	LIST(APPEND ${_target}_SOURCES ${_sources})
	
	GET_DIRECTORY_PROPERTY(_inc_dirs INCLUDE_DIRECTORIES)
	FILE(GLOB _inc_dirs ${_inc_dirs} .)
	LIST(APPEND ${_target}_INC_DIRS ${_inc_dirs})
	
	# If there is a parent scope, set these variables there as well.
	GET_DIRECTORY_PROPERTY(_parent_dir PARENT_DIRECTORY)
	IF(_parent_dir)
		SET(${_target}_SOURCES ${${_target}_SOURCES} PARENT_SCOPE)
		SET(${_target}_INC_DIRS ${${_target}_INC_DIRS} PARENT_SCOPE)
	ENDIF()
ENDMACRO()


# UPDATE_TRANSLATION_SET(<target> <*.ts files>)
#
# Creates the update_translations_${target} target to parse source files
# added by TRANSLATION_SOURCES() for translatable strings.
# May be called multiple times for different targets.
# Optionally to be followed by UPDATE_TRANSLATIONS()
#
MACRO(UPDATE_TRANSLATION_SET _target) #, ts_files
	SET(_sources_str "")
	FOREACH(_file ${${_target}_SOURCES})
		GET_FILENAME_COMPONENT(_abs "${_file}" ABSOLUTE)
		SET(_sources_str "${_sources_str} \"${_abs}\"")
	ENDFOREACH()
	
	SET(_inc_dirs ${${_target}_INC_DIRS})
	LIST(REMOVE_DUPLICATES _inc_dirs)
	
	UNSET(_filtered_inc_dirs)
	FOREACH(_dir ${_inc_dirs})
		# We are going to accept include directories within our
		# source and binary trees and reject all others.  Allowing lupdate
		# to parse things like boost headers leads to spurious warnings.
		FILE(RELATIVE_PATH _dir_rel_to_source "${CMAKE_SOURCE_DIR}" "${_dir}")
		FILE(RELATIVE_PATH _dir_rel_to_binary "${CMAKE_BINARY_DIR}" "${_dir}")
		IF(NOT _dir_rel_to_source MATCHES "\\.\\..*")
			LIST(APPEND _filtered_inc_dirs "${_dir}")
		ELSEIF(NOT _dir_rel_to_binary MATCHES "\\.\\..*")
			LIST(APPEND _filtered_inc_dirs "${_dir}")
		ENDIF()
	ENDFOREACH()
	
	SET(_inc_dirs_str "")
	FOREACH(_dir ${_filtered_inc_dirs})
		SET(_inc_dirs_str "${_inc_dirs_str} \"${_dir}\"")
	ENDFOREACH()
	
	SET(_translations_str "")
	FOREACH(_file ${ARGN})
		GET_FILENAME_COMPONENT(_abs "${_file}" ABSOLUTE)
		SET(_translations_str "${_translations_str} \"${_abs}\"")
	ENDFOREACH(_file)

	FILE(
		WRITE "${CMAKE_BINARY_DIR}/update_translations_${_target}.pro"
		"SOURCES = ${_sources_str}\nTRANSLATIONS = ${_translations_str}\nINCLUDEPATH = ${_inc_dirs_str}"
	)

	ADD_CUSTOM_TARGET(
		update_translations_${_target}
		COMMAND "${QT_LUPDATE_EXECUTABLE}" -locations absolute
		-pro "${CMAKE_BINARY_DIR}/update_translations_${_target}.pro"
		SOURCES ${ARGN}
		VERBATIM
	)
ENDMACRO()


# UPDATE_TRANSLATIONS(<targets>)
#
# Creates the update_translations target and makes it dependent on
# targets created by UPDATE_TRANSLATION_SET()
#
MACRO(UPDATE_TRANSLATIONS) # targets
	ADD_CUSTOM_TARGET(update_translations)
	FOREACH(_target ${ARGN})
		ADD_DEPENDENCIES(update_translations update_translations_${_target})
	ENDFOREACH()
ENDMACRO()