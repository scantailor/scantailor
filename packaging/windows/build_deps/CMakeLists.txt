CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE
	"${CMAKE_SOURCE_DIR}/../../../cmake/default_cflags.cmake"
)
SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
	"${CMAKE_SOURCE_DIR}/../../../cmake/default_cxxflags.cmake"
)

PROJECT("Scan Tailor Dependencies")

INCLUDE(../../../cmake/SetDefaultBuildType.cmake)
INCLUDE(../../../cmake/SetDefaultGccFlags.cmake)
INCLUDE(../../../cmake/ToNativePath.cmake)
INCLUDE(../../../cmake/PatchFile.cmake)
INCLUDE(TestCXXAcceptsFlag)

ST_SET_DEFAULT_BUILD_TYPE(Release)
ST_SET_DEFAULT_GCC_FLAGS()

GET_FILENAME_COMPONENT(source_outer_dir "${PROJECT_SOURCE_DIR}/../../../.." ABSOLUTE)
GET_FILENAME_COMPONENT(build_outer_dir "${PROJECT_BINARY_DIR}/.." ABSOLUTE)

FILE(GLOB jpeg_dirs1 "${build_outer_dir}/jpeg-[0-9]*")
FILE(GLOB jpeg_dirs2 "${source_outer_dir}/jpeg-[0-9]*")
FILE(GLOB zlib_dirs1 "${build_outer_dir}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB zlib_dirs2 "${source_outer_dir}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs1 "${build_outer_dir}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs2 "${source_outer_dir}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs3 "${build_outer_dir}/lpng[0-9]*")
FILE(GLOB png_dirs4 "${source_outer_dir}/lpng[0-9]*")
FILE(GLOB tiff_dirs1 "${build_outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB tiff_dirs2 "${source_outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB qt_dirs1 "${build_outer_dir}/qt-win-*-5.[0-9]*")
FILE(GLOB qt_dirs2 "${build_outer_dir}/qt-everywhere-*-5.[0-9]*")
FILE(GLOB qt_dirs3 "${source_outer_dir}/qt-win-*-5.[0-9]*")
FILE(GLOB qt_dirs4 "${source_outer_dir}/qt-everywhere-*-5.[0-9]*")
FILE(GLOB qt_dirs5 "C:/Qt/Qt5.*/*/Src")
FILE(GLOB boost_dirs1 "${build_outer_dir}/boost_1_[0-9]*_[0-9]*")
FILE(GLOB boost_dirs2 "${source_outer_dir}/boost_1_[0-9]*_[0-9]*")

FIND_PATH(
	JPEG_DIR jpeglib.h HINTS ${jpeg_dirs1} ${jpeg_dirs2}
	DOC "Path to jpeg source directory."
)
FIND_PATH(
	ZLIB_DIR zlib.h HINTS ${zlib_dirs1} ${zlib_dirs2}
	DOC "Path to zlib source directory."
)
FIND_PATH(
	PNG_DIR png.h HINTS ${png_dirs1} ${png_dirs2} ${png_dirs3} ${png_dirs4}
	DOC "Path to libpng source directory."
)
FIND_PATH(
	TIFF_DIR libtiff/tiff.h HINTS ${tiff_dirs1} ${tiff_dirs2}
	DOC "Path to top-level tiff source directory."
)
FIND_PATH(
	QT_SRC_DIR NAMES qtbase/qtbase.pro HINTS ${qt_dirs1} ${qt_dirs2}
	${qt_dirs3} ${qt_dirs4} ${qt_dirs5}
	DOC "Path to top-level Qt5 source directory. If you installed a binary version, it will be something like C:\\Qt\\Qt5.0.2\\5.0.2\\Src"
)
FIND_PATH(
	BOOST_DIR boost/foreach.hpp HINTS ${boost_dirs1} ${boost_dirs2}
	DOC "Path to top-level Boost source directory."
)

IF(NOT JPEG_DIR)
	MESSAGE(FATAL_ERROR "JPEG source directory not found. You may specify it manually.")
ELSEIF(NOT ZLIB_DIR)
	MESSAGE(FATAL_ERROR "ZLIB source directory not found. You may specify it manually.")
ELSEIF(NOT PNG_DIR)
	MESSAGE(FATAL_ERROR "LibPNG source directory not found. You may specify it manually.")
ELSEIF(NOT TIFF_DIR)
	MESSAGE(FATAL_ERROR "TIFF source directory not found. You may specify it manually.")
ELSEIF(NOT QT_SRC_DIR)
	MESSAGE(FATAL_ERROR "Qt5 source directory not found. You may specify it manually. If you installed a binary version, it will be something like C:\\Qt\\Qt5.0.2\\5.0.2\\Src")
ELSEIF(NOT BOOST_DIR)
	MESSAGE(FATAL_ERROR "Boost source directory not found. You may specify it manually.")
ENDIF()

SET(QTBASE_DIR "${QT_SRC_DIR}/qtbase")

FILE(GLOB qt_possible_prebuilt_dirs "${QT_SRC_DIR}/../*")
FIND_PATH(
	QT_PREBUILT_DIR bin/qmake.exe HINTS ${qt_possible_prebuilt_dirs}
	DOC "[optional] Installation path of a pre-built version of Qt5. If you installed a binary version, it will be something like C:\\Qt\\Qt5.0.2\\5.0.2\\msvc2012"
)

SET(ZLIB_LIBRARY_NAME zdll)
SET(PNG_LIBRARY_NAME libpng)
SET(JPEG_LIBRARY_NAME libjpeg)
SET(TIFF_LIBRARY_NAME libtiff)

MACRO(LIST_ITEMS_PREPEND LIST PREFIX)
	SET(tmp_list_)
	FOREACH(item ${${LIST}})
		LIST(APPEND tmp_list_ "${PREFIX}${item}")
	ENDFOREACH(item)
	SET(${LIST} ${tmp_list_})
ENDMACRO(LIST_ITEMS_PREPEND)


#=================================== JPEG ===================================#

# Patch jmorecfg.h to:
# 1. Prevent double definition of INT32.
# 2. Build a DLL rather than a static library.
IF(NOT EXISTS "${JPEG_DIR}/jmorecfg.h.orig")
	FILE(READ "${JPEG_DIR}/jmorecfg.h" jmorecfg_h_orig)
	STRING(REPLACE "XMD_H" "_BASETSD_H" jmorecfg_h "${jmorecfg_h_orig}")
	STRING(
		REGEX REPLACE "#define[ \t]+GLOBAL\\(type\\)[^\n]*"
		"#ifdef JPEG_BUILD\n#define GLOBAL(type) __declspec(dllexport) type\n#else\n#define GLOBAL(type) __declspec(dllimport) type\n#endif"
		jmorecfg_h "${jmorecfg_h}"
	)
	STRING(
		REGEX REPLACE "#define[ \t]+EXTERN\\(type\\)[^\n]*"
		"#ifdef JPEG_BUILD\n#define EXTERN(type) extern __declspec(dllexport) type\n#else\n#define EXTERN(type) extern __declspec(dllimport) type\n#endif"
		jmorecfg_h "${jmorecfg_h}"
	)
	FILE(WRITE "${JPEG_DIR}/jmorecfg.h" "${jmorecfg_h}")
	FILE(WRITE "${JPEG_DIR}/jmorecfg.h.orig" "${jmorecfg_h_orig}")
	SET(jmorecfg_h "")
	SET(jmorecfg_h_orig "")
ENDIF(NOT EXISTS "${JPEG_DIR}/jmorecfg.h.orig")

SET(
	libjpeg_sources
	jaricom.c jcapimin.c jcapistd.c jcarith.c jccoefct.c jccolor.c
	jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c
	jcomapi.c jcparam.c jcprepct.c jcsample.c jctrans.c jdapimin.c
	jdapistd.c jdarith.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c
	jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c
	jdmerge.c jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c
	jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jquant1.c
	jquant2.c jutils.c jmemmgr.c jmemnobs.c
)
LIST_ITEMS_PREPEND(libjpeg_sources "${JPEG_DIR}/")

CONFIGURE_FILE("${JPEG_DIR}/jconfig.vc" "${JPEG_DIR}/jconfig.h" COPYONLY)

ADD_LIBRARY(${JPEG_LIBRARY_NAME} SHARED ${libjpeg_sources})
SET_TARGET_PROPERTIES(
	${JPEG_LIBRARY_NAME} PROPERTIES
	DEFINE_SYMBOL JPEG_BUILD
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/staging/libs"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${QTBASE_DIR}/bin"
)


#=================================== ZLIB ===================================#

SET(
	zlib_sources
	adler32.c compress.c crc32.c deflate.c inffast.c inflate.c
	inftrees.c trees.c uncompr.c zutil.c
)
LIST_ITEMS_PREPEND(zlib_sources "${ZLIB_DIR}/")

ADD_LIBRARY(${ZLIB_LIBRARY_NAME} SHARED ${zlib_sources})
SET_TARGET_PROPERTIES(
	${ZLIB_LIBRARY_NAME} PROPERTIES
	DEFINE_SYMBOL ZLIB_DLL
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/staging/libs"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${QTBASE_DIR}/bin"
)


#================================== LIBPNG ==================================#

IF(NOT EXISTS "${PNG_DIR}/pnglibconf.h")
	IF(EXISTS "${PNG_DIR}/scripts/pnglibconf.h.prebuilt")
		CONFIGURE_FILE(
			"${PNG_DIR}/scripts/pnglibconf.h.prebuilt"
			"${PNG_DIR}/pnglibconf.h" @COPYONLY
		)
	ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${ZLIB_DIR})

SET(
	libpng_sources
	png.c pngset.c pngget.c pngrutil.c pngtrans.c pngwutil.c pngread.c
	pngrio.c pngwio.c pngwrite.c pngrtran.c pngwtran.c pngmem.c
	pngerror.c pngpread.c
)
LIST_ITEMS_PREPEND(libpng_sources "${PNG_DIR}/")

ADD_DEFINITIONS(-DPNG_NO_MODULEDEF)
ADD_LIBRARY(${PNG_LIBRARY_NAME} SHARED ${libpng_sources})
TARGET_LINK_LIBRARIES(${PNG_LIBRARY_NAME} ${ZLIB_LIBRARY_NAME})
SET_TARGET_PROPERTIES(
	${PNG_LIBRARY_NAME} PROPERTIES
	DEFINE_SYMBOL PNG_BUILD_DLL
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/staging/libs"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${QTBASE_DIR}/bin"
)


#=================================== TIFF ===================================#

INCLUDE_DIRECTORIES("${JPEG_DIR}") # ZLIB_DIR already included above

SET(
	libtiff_sources
	tif_aux.c tif_close.c tif_codec.c tif_color.c tif_compress.c
	tif_dir.c tif_dirinfo.c tif_dirread.c tif_dirwrite.c
	tif_dumpmode.c tif_error.c tif_extension.c tif_fax3.c
	tif_fax3sm.c tif_getimage.c tif_jpeg.c tif_ojpeg.c tif_flush.c
	tif_luv.c tif_lzw.c tif_next.c tif_open.c tif_packbits.c
	tif_pixarlog.c tif_predict.c tif_print.c tif_read.c tif_swab.c
	tif_strip.c tif_thunder.c tif_tile.c tif_version.c
	tif_warning.c tif_write.c tif_zip.c tif_unix.c libtiff.def
)
LIST_ITEMS_PREPEND(libtiff_sources "${TIFF_DIR}/libtiff/")

SET(tiff_vc_config "${TIFF_DIR}/libtiff/tif_config.vc.h")
IF(EXISTS "${TIFF_DIR}/libtiff/tif_config.h.vc")
	SET(tiff_vc_config "${TIFF_DIR}/libtiff/tif_config.h.vc")
ENDIF(EXISTS "${TIFF_DIR}/libtiff/tif_config.h.vc")
CONFIGURE_FILE(
	"${tiff_vc_config}" "${TIFF_DIR}/libtiff/tif_config.h" @COPYONLY
)
IF(EXISTS "${TIFF_DIR}/libtiff/tiffconf.vc.h")
	CONFIGURE_FILE(
		"${TIFF_DIR}/libtiff/tiffconf.vc.h"
		"${TIFF_DIR}/libtiff/tiffconf.h" @COPYONLY
	)
ENDIF(EXISTS "${TIFF_DIR}/libtiff/tiffconf.vc.h")

ADD_LIBRARY(${TIFF_LIBRARY_NAME} SHARED ${libtiff_sources})
TARGET_LINK_LIBRARIES(
	${TIFF_LIBRARY_NAME}
	${PNG_LIBRARY_NAME} ${JPEG_LIBRARY_NAME} ${ZLIB_LIBRARY_NAME}
)
SET_TARGET_PROPERTIES(
	${TIFF_LIBRARY_NAME} PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/staging/libs"
	ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/staging/libs"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${QTBASE_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${QTBASE_DIR}/bin"
)


#================================= Boost ================================#

IF(MSVC)
	SET(boost_toolset msvc)
ELSE()
	MESSAGE(FATAL_ERROR "Unsupported platform. Only Visual Studio is currently supported")
ENDIF()

SET(boost_64bit_flags "")
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	LIST(APPEND boost_64bit_flags "address-model=64")
ENDIF()

ADD_CUSTOM_TARGET(
	boost ALL
	COMMAND cmd /c bootstrap
	COMMAND b2 --with-test toolset=${boost_toolset}
	threading=multi link=static variant=debug,release ${boost_64bit_flags}
	stage
	WORKING_DIRECTORY "${BOOST_DIR}"
	VERBATIM
)


#=================================== Qt ===================================#

# Backward compatibility.  We used to patch projects.pro, but now we achieve
# the same result by other means.
IF(EXISTS "${QTBASE_DIR}/projects.pro.orig")
	CONFIGURE_FILE(
		"${QTBASE_DIR}/projects.pro.orig"
		"${QTBASE_DIR}/projects.pro" COPYONLY
	)
	FILE(REMOVE "${QTBASE_DIR}/projects.pro.orig")
ENDIF(EXISTS "${QTBASE_DIR}/projects.pro.orig")


TO_NATIVE_PATH("${JPEG_DIR}" JPEG_INCLUDE_DIR)
TO_NATIVE_PATH("${ZLIB_DIR}" ZLIB_INCLUDE_DIR)
TO_NATIVE_PATH("${PNG_DIR}" PNG_INCLUDE_DIR)
TO_NATIVE_PATH("${QTBASE_DIR}" QTBASE_DIR_NATIVE)


# Find all *.conf files under mkspecs that mention -Zc:wchar_t- and remove
# that minus at the end.  That's necessary to make Qt compatible with other
# libraries that use wchar_t stuff.
FILE(GLOB_RECURSE conf_files "${QTBASE_DIR}/mkspecs/*.conf")
FOREACH(conf_file ${conf_files})
	FILE(READ "${conf_file}" contents)
	STRING(REGEX REPLACE "-Zc:wchar_t-" "-Zc:wchar_t" new_contents "${contents}")
	IF(NOT "${contents}" STREQUAL "${new_contents}")
		# Make a backup copy, if not already there.
		IF(NOT EXISTS "${conf_file}.orig")
			CONFIGURE_FILE("${conf_file}" "${conf_file}.orig" COPYONLY)
		ENDIF()

		FILE(WRITE "${conf_file}" "${new_contents}")
	ENDIF()
ENDFOREACH()

IF(MSVC)
	SET(make_command nmake)
	IF(MSVC60 OR MSVC70 OR MSVC71 OR MSVC80 OR MSVC90)
		MESSAGE(FATAL_ERROR "This Visual Studio version is too old and is not supported")
	ELSEIF(MSVC100)
		MESSAGE("Visual Studio 2010 may work but wasn't tested. Visual Studio 2012 is strongly recommended")
		SET(platform win32-msvc2010)
		SET(force_frame_pointer TRUE)
	ELSE()
		# Visual Studio 2012 or above
		SET(platform win32-msvc2012)
		SET(force_frame_pointer TRUE)
	ENDIF()
ELSE()
	MESSAGE(FATAL_ERROR "Unsupported platform.")
ENDIF()


IF(MSVC AND force_frame_pointer)
	# Disable frame pointer ommission in release mode to ensure
	# good stack traces from our built-in crash reporter.
	PATCH_FILE(
		"${QTBASE_DIR}/mkspecs/${platform}/qmake.conf"
		
		# Add the flag.
		"/(QMAKE_CFLAGS_RELEASE[ \t]*=)/\\1 -Oy-/"
		
		# Get rid of duplicates, in case it was already there.
		"/( -Oy-)+/ -Oy-/"
	)
ENDIF()
	

# Force debugging symbols even for release builds.
# Qt5 has a -force-debug-info configure option
# but I had to do the following to make it actually work:
PATCH_FILE(
	"${QTBASE_DIR}/.qmake.conf"
	
	# Put "CONFIG += force_debug_info" at the end.
	"/(.)$/\\1\nCONFIG += force_debug_info\n/"
)


SET(qt_build_script "${CMAKE_BINARY_DIR}/build-qt.bat")

SET(maybe_skip_building_tools "")
IF(QT_PREBUILT_DIR)
	SET(maybe_skip_building_tools "goto exit")
ENDIF()

ADD_CUSTOM_COMMAND(
	OUTPUT "${qt_build_script}"
	COMMAND "${CMAKE_COMMAND}"
	"-DPROJECT_ROOT=${CMAKE_SOURCE_DIR}/../../.."
	"-DTARGET_FILE=${qt_build_script}"
	"-DPLATFORM=${platform}"
	"-DMAYBE_SKIP_BUILDING_TOOLS=${maybe_skip_building_tools}"
	"-DMAKE_COMMAND=${make_command}"
	"-DJPEG_INCLUDE_DIR=${JPEG_INCLUDE_DIR}"
	"-DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}"
	"-DPNG_INCLUDE_DIR=${PNG_INCLUDE_DIR}"
	"-DJPEG_LINK_DIR=$<TARGET_LINKER_FILE_DIR:${JPEG_LIBRARY_NAME}>"
	"-DZLIB_LINK_DIR=$<TARGET_LINKER_FILE_DIR:${ZLIB_LIBRARY_NAME}>"
	"-DPNG_LINK_DIR=$<TARGET_LINKER_FILE_DIR:${PNG_LIBRARY_NAME}>"
	-P "${CMAKE_CURRENT_SOURCE_DIR}/generate_qt_build_script.cmake"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generate_qt_build_script.cmake"
	VERBATIM
)
	
ADD_CUSTOM_TARGET(
	Qt ALL
	COMMAND "${qt_build_script}"
	DEPENDS "${qt_build_script}"
	WORKING_DIRECTORY "${QTBASE_DIR}"
)

# boost added so that boost is built before Qt. That's helpful if build
# errors occur, as rebuilding boost is much faster than rebuilding Qt.
ADD_DEPENDENCIES(
	Qt ${ZLIB_LIBRARY_NAME} ${JPEG_LIBRARY_NAME}
	${PNG_LIBRARY_NAME} ${TIFF_LIBRARY_NAME}
)

# If you have multiple versions of Qt installed, this script
# can easily pick the wrong one.
MESSAGE(STATUS "----------------------------")
MESSAGE(STATUS "QT_SRC_DIR is ${QT_SRC_DIR}")
MESSAGE(STATUS "Check it's the one you want.")
IF(QT_PREBUILT_DIR)
	MESSAGE(STATUS "QT_PREBUILT_DIR is ${QT_PREBUILT_DIR}")
ELSE()
	MESSAGE(STATUS "QT_PREBUILT_DIR is not set. Using a source-only version of Qt? Fine, but building will take longer.")
ENDIF()
MESSAGE(STATUS "----------------------------")


# Dump symbols of all DLLs we are going to link to.
OPTION(DUMP_DLL_SYMBOLS "Enable if you are going to build crash reporter." OFF)
IF(DUMP_DLL_SYMBOLS)
	FIND_PATH(
		SYMBOLS_PATH . PATHS "${build_outer_dir}/symbols"
		"${source_outer_dir}/symbols" NO_DEFAULT_PATH
		DOC "The directory to write symbol information into."
	)
	IF(NOT SYMBOLS_PATH)
		MESSAGE(FATAL_ERROR "SYMBOLS_PATH directory is not set.")
	ENDIF(NOT SYMBOLS_PATH)

	# We can't build it, because it requires ATL, which is not part
	# of the Visual Studio Express Edition, so we rely on a pre-built
	# version which can be found in the Mozilla repository.
	FIND_PROGRAM(
		DUMP_SYMS_EXECUTABLE dump_syms PATHS "${build_outer_dir}" "${source_outer_dir}"
		DOC "Path to dump_syms.exe, which can be found in Mozilla repository."
	)
	IF(NOT DUMP_SYMS_EXECUTABLE)
		MESSAGE(
			FATAL_ERROR "dump_syms.exe wasn't found.  Specify its location manually by setting the DUMP_SYMS_EXECUTABLE variable.  dump_syms.exe may be found in the Mozilla repository under /toolkit/crashreporter/tools/win32"
		)
	ENDIF(NOT DUMP_SYMS_EXECUTABLE)
	
	ADD_CUSTOM_TARGET(
		dump_dll_symbols ALL
		COMMAND "${CMAKE_COMMAND}" "-DSYMBOLS_PATH=${SYMBOLS_PATH}"
		"-DDUMP_SYMS_EXECUTABLE=${DUMP_SYMS_EXECUTABLE}"
		"-DMOVE_SYMBOLS_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/move_sym_file.cmake"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/dump_dll_syms.cmake"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/dump_dll_syms.cmake"
		WORKING_DIRECTORY "${QTBASE_DIR}/bin"
		VERBATIM
	)
	
	# Qt depends on the rest of them.
	ADD_DEPENDENCIES(dump_dll_symbols Qt)
ENDIF()
