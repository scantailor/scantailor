# Applies a list of search / replace expressions to a file.
# If the file is modified, saves a backup with .orig name
# appended to the original file name.  If a backup already
# exists, it's not overwritten.
# Example usage:
# PATCH_FILE("C:/test.txt" "/apple/orange/" "/@([A-Z_]+)@/@\\1_OLD@")
FUNCTION(PATCH_FILE file_)
	MESSAGE(STATUS "Patching file ${file_}")
	
	IF(EXISTS "${file_}.orig")
		FILE(READ "${file_}.orig" contents_)
	ELSE()
		FILE(READ "${file_}" contents_)
	ENDIF()
	SET(orig_contents "${contents_}")
	
	FOREACH(search_replace_ ${ARGN})
		IF(search_replace_ STREQUAL "")
			MESSAGE(FATAL_ERROR "Empty string passed as a search/replace expression to PATCH_FILE()")
		ENDIF()
		
		# Parse the string of type /search/replace/ into search_ and _replace variables.
		# Note that any symbol missing from both search and replace can be used instead of /.
		STRING(SUBSTRING "${search_replace_}" 0 1 sep_)
		STRING(REGEX REPLACE "^${sep_}([^${sep_}]+)${sep_}.*" "\\1" search_ "${search_replace_}")
		STRING(REGEX REPLACE "^${sep_}[^${sep_}]+${sep_}([^${sep_}]*)${sep_}\$" "\\1" replace_ "${search_replace_}")
		IF(search_replace_ STREQUAL "${replace_}")
			MESSAGE(FATAL_ERROR "Invalid search/replace expression passed to PATCH_FILE()")
		ENDIF()
		
		STRING(REGEX REPLACE "${search_}" "${replace_}" contents_ "${contents_}")
	ENDFOREACH()
	
	IF(NOT "${contents_}" STREQUAL "${orig_contents_}")
		# Make a backup copy, if not already there.
		IF(NOT EXISTS "${file_}.orig")
			CONFIGURE_FILE("${file_}" "${file_}.orig" COPYONLY)
		ENDIF()

		FILE(WRITE "${file_}" "${contents_}")
	ENDIF()
ENDFUNCTION()
