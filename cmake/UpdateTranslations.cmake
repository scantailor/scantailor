# TRANSLATION_SOURCES(<translation set> <source files>)
#
# Associates the specified source files with a translation set.  A translation
# set corresponds to a family of *.ts files with the same prefix, one for each
# translation.  Usually translation sets also correspond to build targets,
# though they don't have to.  You may use the target name as a translation set
# name if you want to.  Translation set names can only contain characters
# allowed in CMake variable names.
# This macro may be called multiple times, possibly from different directories.
# The typical usage will be like this:
#
# TRANSLATION_SOURCES(myapp MainWindow.cpp MainWindow.h MainWindow.ui ...)
# FINALIZE_TRANSLATION_SET(myapp myapp_de.ts myapp_ru.ts myapp_ja.ts ...)
# UPDATE_TRANSLATIONS_TARGET(update_translations myapp)
#
MACRO(TRANSLATION_SOURCES _set) #, _sources
	FILE(GLOB _sources ABSOLUTE ${ARGN})
	LIST(APPEND ${_set}_SOURCES ${_sources})
	
	GET_DIRECTORY_PROPERTY(_inc_dirs INCLUDE_DIRECTORIES)
	FILE(GLOB _inc_dirs ${_inc_dirs} .)
	LIST(APPEND ${_set}_INC_DIRS ${_inc_dirs})
	
	# If there is a parent scope, set these variables there as well.
	GET_DIRECTORY_PROPERTY(_parent_dir PARENT_DIRECTORY)
	IF(_parent_dir)
		SET(${_set}_SOURCES ${${_set}_SOURCES} PARENT_SCOPE)
		SET(${_set}_INC_DIRS ${${_set}_INC_DIRS} PARENT_SCOPE)
	ENDIF()
ENDMACRO()


# FINALIZE_TRANSLATION_SET(<translation set>, <*.ts files>)
#
# Associates *.ts files with a translation set.
# May be called multiple times for different translation sets.
# To be followed by UPDATE_TRANSLATIONS_TARGET()
#
MACRO(FINALIZE_TRANSLATION_SET _set) #, _ts_files
	SET(_sources_str "")
	FOREACH(_file ${${_set}_SOURCES})
		SET(_sources_str "${_sources_str} \"${_file}\"")
	ENDFOREACH()
	
	SET(_inc_dirs ${${_set}_INC_DIRS})
	LIST(REMOVE_DUPLICATES _inc_dirs)
	
	SET(_filtered_inc_dirs "")
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
		WRITE "${CMAKE_BINARY_DIR}/update_translations_${_set}.pro"
		"SOURCES = ${_sources_str}\nTRANSLATIONS = ${_translations_str}\nINCLUDEPATH = ${_inc_dirs_str}"
	)

	# Note that we can't create a custom target with *.ts files as output, because:
	# 1. CMake would pollute our source tree with *.rule fules.
	# 2. "make clean" would remove them.
ENDMACRO()


# UPDATE_TRANSLATIONS_TARGET(<target> <translation sets>)
#
# Creates a target that updates *.ts files assiciated with the specified
# translation sets by FINALIZE_TRANSLATION_SET()
#
MACRO(UPDATE_TRANSLATIONS_TARGET _target) #, _sets
	SET(_commands "")
	FOREACH(_set ${ARGN})
		LIST(
			APPEND _commands COMMAND "${QT_LUPDATE_EXECUTABLE}" -locations absolute
			-pro "${CMAKE_BINARY_DIR}/update_translations_${_set}.pro"
		)
	ENDFOREACH()
	
	ADD_CUSTOM_TARGET(${_target} ${_commands} VERBATIM)
ENDMACRO()