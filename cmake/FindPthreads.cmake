MACRO(FindPthreads)
	SET(PTHREADS_FOUND FALSE)
	
	# This won't overwrite values already in cache.
	SET(PTHREADS_CFLAGS "" CACHE STRING "Compiler flags for pthreads")
	SET(PTHREADS_LIBS "" CACHE STRING "Linker flags for pthreads")
	MARK_AS_ADVANCED(CLEAR PTHREADS_CFLAGS PTHREADS_LIBS)
	
	SET(_available_flags "")
	
	IF(PTHREADS_CFLAGS OR PTHREADS_LIBS)
		# First try user specified flags.
		LIST(APPEND _available_flags "${PTHREADS_CFLAGS}:${PTHREADS_LIBS}")
	ENDIF(PTHREADS_CFLAGS OR PTHREADS_LIBS)
	
	# -pthreads for gcc, -lpthread for Sun's compiler.
	# Note that there are non-functional stubs of pthread functions
	# in Solaris' libc, so these checks must be done before others.
	SET(_solaris_flags "-pthreads:-pthreads" "-D_REENTRANT:-lpthread")
	
	# No flags required.  This means this check has to be the first
	# on Darwin / Mac OS X, because the compiler will accept almost
	# any flag.
	SET(_darwin_flags ":")
	
	# Must be checked before -lpthread on AIX.
	SET(_aix_flags "-D_THREAD_SAFE:-lpthreads")
	
	# gcc on various OSes
	SET(_other_flags "-pthread:-pthread")
	
	IF(CMAKE_SYSTEM_NAME MATCHES "AIX.*")
		LIST(APPEND _available_flags ${_aix_flags})
		SET(_aix_flags "")
	ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Solaris.*")
		LIST(APPEND _available_flags ${_solaris_flags})
		SET(_solaris_flags "")
	ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin.*")
		LIST(APPEND _available_flags ${_darwin_flags})
		SET(_darwin_flags "")
	ELSE(CMAKE_SYSTEM_NAME MATCHES "AIX.*")
		LIST(APPEND _available_flags ${_other_flags})
		SET(_other_flags "")
	ENDIF(CMAKE_SYSTEM_NAME MATCHES "AIX.*")
	
	LIST(
		APPEND _available_flags
		${_darwin_flags} ${_aix_flags} ${_solaris_flags} ${_other_flags}
	)
	
	LIST(LENGTH _available_flags _num_available_flags)
	SET(_flags_idx 0)
	WHILE(_flags_idx LESS _num_available_flags AND NOT PTHREADS_FOUND)
		LIST(GET _available_flags ${_flags_idx} _flag)
		MATH(EXPR _flags_idx "${_flags_idx} + 1")
		
		STRING(REGEX REPLACE ":.*" "" _cflags "${_flag}")
		STRING(REGEX REPLACE ".*:" "" _libs "${_flag}")
		
		FILE(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestPthreads.c
			"#include <pthread.h>\n"
			"int main()\n"
			"{\n"
			"	pthread_t th;\n"
			"	pthread_create(&th, 0, 0, 0);\n"
			"	pthread_join(th, 0);\n"
			"	pthread_attr_init(0);\n"
			"	pthread_cleanup_push(0, 0);\n"
			"	pthread_cleanup_pop(0);\n"
			"   return 0;\n"
			"}\n"
		)
		
		TRY_COMPILE(
			PTHREADS_FOUND "${CMAKE_BINARY_DIR}"
			${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestPthreads.c
			CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${_libs}"
			COMPILE_DEFINITIONS "${_cflags}"
			OUTPUT_VARIABLE _out
		)
		IF(PTHREADS_FOUND)
			MESSAGE(STATUS "Checking pthreads with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\" -- yes")
			SET(PTHREADS_CFLAGS ${_cflags} CACHE STRING "Compiler flags for pthreads" FORCE)
			SET(PTHREADS_LIBS ${_libs} CACHE STRING "Linker flags for pthreads" FORCE)
			MARK_AS_ADVANCED(FORCE PTHREADS_CFLAGS PTHREADS_LIBS)
		ELSE(PTHREADS_FOUND)
			MESSAGE(STATUS "Checking pthreads with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\" -- no")
			FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"Pthreads don't work with CFLAGS=\"${_cflags}\" and LIBS=\"${_libs}\"\n"
				"Build output follows:\n"
				"==========================================\n"
				"${_out}\n"
				"==========================================\n"
			)
		ENDIF(PTHREADS_FOUND)
	ENDWHILE(_flags_idx LESS _num_available_flags AND NOT PTHREADS_FOUND)
	
ENDMACRO(FindPthreads)
