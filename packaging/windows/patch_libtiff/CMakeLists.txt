CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

PROJECT(patch_libtiff)

GET_FILENAME_COMPONENT(outer_dir "${PROJECT_SOURCE_DIR}/../../../.." ABSOLUTE)

FILE(GLOB tiff_dirs "${outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FIND_PATH(
	TIFF_DIR libtiff/tiff.h PATHS ${tiff_dirs}
	DOC "Path to top-level tiff source directory."
)

IF(NOT DEBIAN_LIBTIFF_PATCH)
	FILE(GLOB tiff_patches "${outer_dir}/tiff*.diff")
	IF(tiff_patches)
		LIST(GET tiff_patches 0 tiff_patch)
		SET(
			DEBIAN_LIBTIFF_PATCH "${tiff_patch}" CACHE FILEPATH
			"Path to the uncompressed .diff file" FORCE
		)
	ENDIF(tiff_patches)
ENDIF(NOT DEBIAN_LIBTIFF_PATCH)

FIND_PROGRAM(
	PATCH_EXE patch
	PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32;InstallPath]/bin"
	DOC "Full path to patch.exe"
)

IF(NOT TIFF_DIR)
	MESSAGE(FATAL_ERROR "TIFF source directory not found. You may specify it manually.")
ELSEIF(NOT DEBIAN_LIBTIFF_PATCH)
	MESSAGE(FATAL_ERROR "The uncompressed Debian patch file for libtiff wasn't found.")
ELSEIF(NOT PATCH_EXE)
	MESSAGE(FATAL_ERROR "The patch utility wasn't found.")
ENDIF(NOT TIFF_DIR)

CONFIGURE_FILE(
	apply_individual_patches.cmake.in
	"${CMAKE_BINARY_DIR}/apply_individual_patches.cmake" @ONLY
)

ADD_CUSTOM_TARGET(
	apply_patches ALL
	COMMAND "${PATCH_EXE}" -p1 --binary -i "${DEBIAN_LIBTIFF_PATCH}"
	COMMAND "${CMAKE_COMMAND}" -P
	"${CMAKE_BINARY_DIR}/apply_individual_patches.cmake"
	WORKING_DIRECTORY "${TIFF_DIR}"
)
