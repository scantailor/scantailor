FIND_PROGRAM(
	MAKENSIS_EXE makensis
	PATHS "[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NSIS;InstallLocation]"
)
IF(NOT MAKENSIS_EXE)
	MESSAGE(
		FATAL_ERROR
		"makensis.exe could not be found.\n"
		"makensis.exe is a part of NSIS. Get NSIS from http://nsis.sf.net/"
	)
ENDIF(NOT MAKENSIS_EXE)

IF(MINGW)
	FIND_FILE(
		MINGWM10_DLL mingwm10.dll
		PATHS c:/MinGW/bin ENV PATH
		DOC "Full path to mingwm10.dll"
	)
	IF(NOT MINGWM10_DLL)
		MESSAGE(
			FATAL_ERROR
			"Could not find mingw10.dll\n"				"It should be in the 'bin' subdirectory of your MinGW installation."
		)
	ENDIF(NOT MINGWM10_DLL)
	TO_NATIVE_PATH("${MINGWM10_DLL}" MINGWM10_DLL)
ELSE(MINGW)
	SET(MINGWM10_DLL "")
ENDIF(MINGW)

SET(LICENSE_FILE "${CMAKE_SOURCE_DIR}/GPL3.txt")
SET(INSTALLER_FILENAME "scantailor-${VERSION}-install.exe")

# VERSION_QUAD must be either empty or have a format of x.x.x.x
STRING(
	REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?$"
	VERSION_QUAD "${VERSION}"
)
STRING(
	REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+)$" "\\1.0"
	VERSION_QUAD "${VERSION_QUAD}"
)

CONFIGURE_FILE(scantailor.nsi.in ${CMAKE_BINARY_DIR}/scantailor.nsi @ONLY)

ADD_CUSTOM_COMMAND(
	OUTPUT "${INSTALLER_FILENAME}"
	COMMAND "${MAKENSIS_EXE}" -V2 scantailor.nsi
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/scantailor.nsi
)
ADD_CUSTOM_TARGET(installer ALL DEPENDS "${INSTALLER_FILENAME}")
