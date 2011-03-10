FIND_PROGRAM(
	MAKENSIS_EXE makensis
	PATHS "[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NSIS;InstallLocation]"
)
#IF(NOT MAKENSIS_EXE)
#	MESSAGE(
#		FATAL_ERROR
#		"makensis.exe could not be found.\n"
#		"makensis.exe is a part of NSIS. Get NSIS from http://nsis.sf.net/"
#	)
#ENDIF(NOT MAKENSIS_EXE)

IF(MINGW)
	FIND_FILE(
		MINGWM10_DLL mingwm10.dll
		PATHS c:/MinGW/bin ENV PATH
		DOC "Full path to mingwm10.dll"
	)
	IF(NOT MINGWM10_DLL)
		MESSAGE(
			FATAL_ERROR
			"Could not find mingwm10.dll\n"
			"It should be in the 'bin' subdirectory of your MinGW installation."
		)
	ENDIF(NOT MINGWM10_DLL)
	TO_NATIVE_PATH("${MINGWM10_DLL}" MINGWM10_DLL)
ELSEIF(MSVC)
	GET_FILENAME_COMPONENT(make_program_dir "${CMAKE_MAKE_PROGRAM}" PATH)
	FIND_FILE(
		VC_REDIST_DIR VC/redist PATHS "${make_program_dir}"
		PATH_SUFFIXES .. ../.. DOC "VC/redist directory."
	)
	IF(VC_REDIST_DIR)
		# Get rid of .. components in the path.
		GET_FILENAME_COMPONENT(VC_REDIST_DIR "${VC_REDIST_DIR}" ABSOLUTE)
		SET(VC_REDIST_DIR "${VC_REDIST_DIR}" CACHE PATH "VC/redist directory." FORCE)

		# Find CRT redistributables.
		FILE(GLOB crt_redist_dirs "${VC_REDIST_DIR}/x86/Microsoft.VC*.CRT")
		IF(crt_redist_dirs)
			LIST(GET crt_redist_dirs 0 CRT_REDIST_PATH)
			GET_FILENAME_COMPONENT(CRT_REDIST_DIR "${CRT_REDIST_PATH}" NAME)
		ENDIF(crt_redist_dirs)
	ENDIF(VC_REDIST_DIR)

	IF (NOT CRT_REDIST_PATH)
		MESSAGE(
			FATAL_ERROR
			"Could not find Visual Studio redistributables.\n"
			"They are typically located in a directory like:\n"
			"C:\Program Files\Microsoft Visual Studio 9.0\VC\redist\n"
			"Please set VC_REDIST_DIR accordinally.\n"
			"Specifically, we are after redist\x86\Microsoft.VC*.CRT"
		)
	ENDIF(NOT CRT_REDIST_PATH)

	TO_NATIVE_PATH("${CRT_REDIST_PATH}" CRT_REDIST_PATH)
	TO_NATIVE_PATH("${CRT_REDIST_DIR}" CRT_REDIST_DIR)
ENDIF(MINGW)

SET(LICENSE_FILE "${CMAKE_SOURCE_DIR}/GPL3.txt")
SET(INSTALLER_FILENAME "scantailor-${VERSION}-install.exe")

SET(REGISTER_EXTENSION_NSH "${CMAKE_CURRENT_SOURCE_DIR}/registerExtension.nsh")
TO_NATIVE_PATH("${REGISTER_EXTENSION_NSH}" REGISTER_EXTENSION_NSH)

CONFIGURE_FILE(scantailor.nsi.in ${CMAKE_BINARY_DIR}/scantailor.nsi @ONLY)

GET_TARGET_PROPERTY(SCANTAILOR_EXE scantailor LOCATION)
TO_NATIVE_PATH("${SCANTAILOR_EXE}" SCANTAILOR_EXE)

SET(extra_deps scantailor compile_translations)

SET(DUMP_SYMBOLS_COMMANDS "")
IF(ENABLE_CRASH_REPORTER)
	GET_TARGET_PROPERTY(CRASHREPORTER_EXE CrashReporter LOCATION)
	TO_NATIVE_PATH("${CRASHREPORTER_EXE}" CRASHREPORTER_EXE)
	LIST(APPEND extra_deps CrashReporter)
	SET(
		DUMP_SYMBOLS_COMMANDS
		COMMAND "${CMAKE_COMMAND}" ARGS -E echo "Dumping symbols..."
		COMMAND "${DUMP_SYMS_EXECUTABLE}" ARGS "${SCANTAILOR_EXE}"
		">" "${SYMBOLS_PATH}/temp.sym"
		COMMAND "${CMAKE_COMMAND}" ARGS "-DSYMBOLS_PATH=${SYMBOLS_PATH}"
		-P "${CMAKE_SOURCE_DIR}/cmake/move_sym_file.cmake"
	)
ENDIF(ENABLE_CRASH_REPORTER)

# SCANTAILOR_EXE and CRASHREPORTER_EXE may contain variable references
# that are only known at run time, so we pass them to makensis, rather
# than harcoding them into scantailor.nsi
ADD_CUSTOM_COMMAND(
	OUTPUT "${INSTALLER_FILENAME}"
	${DUMP_SYMBOLS_COMMANDS}
	COMMAND "${MAKENSIS_EXE}" /V2
	"/DSCANTAILOR_EXE=${SCANTAILOR_EXE}"
	"/DCRASHREPORTER_EXE=${CRASHREPORTER_EXE}" scantailor.nsi
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/scantailor.nsi
	DEPENDS ${extra_deps}
	VERBATIM
)

OPTION(BUILD_INSTALLER_BY_DEFAULT "Wheter the \"installer\" target gets built by default" ON)
SET(installer_target installer)
IF(BUILD_INSTALLER_BY_DEFAULT)
	LIST(APPEND installer_target ALL)
ENDIF()
LIST(APPEND installer_target DEPENDS "${INSTALLER_FILENAME}")
ADD_CUSTOM_TARGET(${installer_target})
