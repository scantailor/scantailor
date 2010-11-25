CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE
	"${CMAKE_SOURCE_DIR}/../../../cmake/default_cflags.cmake"
)
SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
	"${CMAKE_SOURCE_DIR}/../../../cmake/default_cxxflags.cmake"
)

PROJECT("Scan Tailor Dependencies")

# This forces Visual Studio 2008 SP1 to actually link to the versions of the
# libraries it ships with!  It's harmless on other platforms.
ADD_DEFINITIONS(-D_BIND_TO_CURRENT_VCLIBS_VERSION=1)

# Qt's -system-libpng, -system-libjpeg, etc are largely broken with regards
# to static libraries, so it expects:
# libpng.lib on MSVC and libpng.lib.a on MinGW.
SET(CMAKE_STATIC_LIBRARY_PREFIX lib)
IF(MINGW)
	SET(CMAKE_STATIC_LIBRARY_SUFFIX .lib.a)
ELSE(MINGW)
	SET(CMAKE_STATIC_LIBRARY_SUFFIX .lib)
ENDIF(MINGW)

INCLUDE(../../../cmake/SetDefaultBuildType.cmake)
INCLUDE(../../../cmake/SetDefaultGccFlags.cmake)
INCLUDE(../../../cmake/ToNativePath.cmake)

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
FILE(GLOB tiff_dirs1 "${build_outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB tiff_dirs2 "${source_outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB qt_dirs1 "${build_outer_dir}/qt-win-*-4.[0-9]*")
FILE(GLOB qt_dirs2 "${build_outer_dir}/qt-everywhere-*-4.[0-9]*")
FILE(GLOB qt_dirs3 "${source_outer_dir}/qt-win-*-4.[0-9]*")
FILE(GLOB qt_dirs4 "${source_outer_dir}/qt-everywhere-*-4.[0-9]*")
FILE(GLOB qt_dirs5 "C:/Qt/4.*.*")
FILE(GLOB qt_dirs6 "C:/Qt/20*/qt")
FILE(GLOB boost_dirs1 "${build_outer_dir}/boost_1_[0-9]*_[0-9]*")
FILE(GLOB boost_dirs2 "${source_outer_dir}/boost_1_[0-9]*_[0-9]*")
FILE(GLOB boost_jam_dirs1 "${build_outer_dir}/boost-jam-*")
FILE(GLOB boost_jam_dirs2 "${source_outer_dir}/boost-jam-*")

FIND_PATH(
	JPEG_DIR jpeglib.h HINTS ${jpeg_dirs1} ${jpeg_dirs2}
	DOC "Path to jpeg source directory."
)
FIND_PATH(
	ZLIB_DIR zlib.h HINTS ${zlib_dirs1} ${zlib_dirs2}
	DOC "Path to zlib source directory."
)
FIND_PATH(
	PNG_DIR png.h HINTS ${png_dirs1} ${png_dirs2}
	DOC "Path to libpng source directory."
)
FIND_PATH(
	TIFF_DIR libtiff/tiff.h HINTS ${tiff_dirs1} ${tiff_dirs2}
	DOC "Path to top-level tiff source directory."
)
FIND_PATH(
	QT_DIR projects.pro HINTS ${qt_dirs1} ${qt_dirs2}
	${qt_dirs3} ${qt_dirs4} ${qt_dirs5} ${qt_dirs6}
	DOC "Path to top-level Qt source directory."
)
FIND_PATH(
	BOOST_DIR boost/foreach.hpp HINTS ${boost_dirs1} ${boost_dirs2}
	DOC "Path to top-level Boost source directory."
)
FIND_PROGRAM(
	BJAM_EXECUTABLE bjam HINTS ${boost_jam_dirs1} ${boost_jam_dirs2}
	DOC "Path to a directory containing bjam executable."
)

IF(NOT JPEG_DIR)
	MESSAGE(FATAL_ERROR "JPEG source directory not found. You may specify it manually.")
ELSEIF(NOT ZLIB_DIR)
	MESSAGE(FATAL_ERROR "ZLIB source directory not found. You may specify it manually.")
ELSEIF(NOT PNG_DIR)
	MESSAGE(FATAL_ERROR "LibPNG source directory not found. You may specify it manually.")
ELSEIF(NOT TIFF_DIR)
	MESSAGE(FATAL_ERROR "TIFF source directory not found. You may specify it manually.")
ELSEIF(NOT QT_DIR)
	MESSAGE(FATAL_ERROR "Qt source directory not found. You may specify it manually.")
ELSEIF(NOT BOOST_DIR)
	MESSAGE(FATAL_ERROR "Boost source directory not found. You may specify it manually.")
ELSEIF(NOT BJAM_EXECUTABLE)
	MESSAGE(FATAL_ERROR "bjam executable not found. You may specify it manually.")
ENDIF(NOT JPEG_DIR)

MACRO(LIST_ITEMS_PREPEND LIST PREFIX)
	SET(tmp_list_)
	FOREACH(item ${${LIST}})
		LIST(APPEND tmp_list_ "${PREFIX}${item}")
	ENDFOREACH(item)
	SET(${LIST} ${tmp_list_})
ENDMACRO(LIST_ITEMS_PREPEND)

#=================================== JPEG ===================================#

# Patch jmorecfg.h to prevent double definition of INT32.
IF(NOT EXISTS "${JPEG_DIR}/jmorecfg.h.orig")
	FILE(READ "${JPEG_DIR}/jmorecfg.h" jmorecfg_h_orig)
	STRING(REPLACE "XMD_H" "_BASETSD_H" jmorecfg_h "${jmorecfg_h_orig}")
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

ADD_LIBRARY(jpeg STATIC ${libjpeg_sources})

GET_TARGET_PROPERTY(jpeg_location jpeg LOCATION)
ADD_CUSTOM_COMMAND(
	TARGET jpeg POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${jpeg_location}" "${JPEG_DIR}/"
)


#=================================== ZLIB ===================================#

SET(
	zlib_sources
	adler32.c compress.c crc32.c deflate.c inffast.c inflate.c
	inftrees.c trees.c uncompr.c zutil.c
)
LIST_ITEMS_PREPEND(zlib_sources "${ZLIB_DIR}/")

ADD_LIBRARY(zlib STATIC ${zlib_sources})
SET_TARGET_PROPERTIES(zlib PROPERTIES OUTPUT_NAME z)
GET_TARGET_PROPERTY(zlib_location zlib LOCATION)
ADD_CUSTOM_COMMAND(
	TARGET zlib POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${zlib_location}" "${ZLIB_DIR}/"
)


#================================== LIBPNG ==================================#

INCLUDE_DIRECTORIES(${ZLIB_DIR})

SET(
	libpng_sources
	png.c pngset.c pngget.c pngrutil.c pngtrans.c pngwutil.c pngread.c
	pngrio.c pngwio.c pngwrite.c pngrtran.c pngwtran.c pngmem.c
	pngerror.c pngpread.c
)
LIST_ITEMS_PREPEND(libpng_sources "${PNG_DIR}/")

ADD_LIBRARY(png STATIC ${libpng_sources})
GET_TARGET_PROPERTY(png_location png LOCATION)
ADD_CUSTOM_COMMAND(
	TARGET png POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${png_location}" "${PNG_DIR}/"
)


#=================================== TIFF ===================================#

INCLUDE_DIRECTORIES(${JPEG_DIR}) # ZLIB_DIR already included above

SET(
	tiff_definitions
	-DAVOID_WIN32_FILEIO -DLIBTIFF_STATIC
)

SET(
	libtiff_sources
	tif_aux.c tif_close.c tif_codec.c tif_color.c tif_compress.c
	tif_dir.c tif_dirinfo.c tif_dirread.c tif_dirwrite.c
	tif_dumpmode.c tif_error.c tif_extension.c tif_fax3.c
	tif_fax3sm.c tif_getimage.c tif_jpeg.c tif_ojpeg.c tif_flush.c
	tif_luv.c tif_lzw.c tif_next.c tif_open.c tif_packbits.c
	tif_pixarlog.c tif_predict.c tif_print.c tif_read.c tif_swab.c
	tif_strip.c tif_thunder.c tif_tile.c tif_version.c
	tif_warning.c tif_write.c tif_zip.c tif_unix.c
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
`	)
ENDIF(EXISTS "${TIFF_DIR}/libtiff/tiffconf.vc.h")

ADD_DEFINITIONS(${tiff_definitions})
ADD_LIBRARY(tiff STATIC ${libtiff_sources})

GET_TARGET_PROPERTY(tiff_location tiff LOCATION)
ADD_CUSTOM_COMMAND(
	TARGET tiff POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${tiff_location}" "${TIFF_DIR}/libtiff/"
)


#================================= Boost ================================#

IF(MINGW)
	SET(boost_toolset gcc)
ELSEIF(MSVC)
	SET(boost_toolset msvc)
ELSE(MINGW)
	MESSAGE(FATAL_ERROR "Unsupported platform.  MinGW and MSVC are supported.")
ENDIF(MINGW)

ADD_CUSTOM_TARGET(
	boost "${BJAM_EXECUTABLE}" --with-test toolset=${boost_toolset}
	threading=multi link=static variant=debug,release
	define=_BIND_TO_CURRENT_VCLIBS_VERSION=1
	stage
	WORKING_DIRECTORY "${BOOST_DIR}"
)


#=================================== Qt ===================================#

# Backward compatibility.  We used to patch projects.pro, but now we achieve
# the same result by other means.
IF(EXISTS "${QT_DIR}/projects.pro.orig")
	CONFIGURE_FILE(
		"${QT_DIR}/projects.pro.orig"
		"${QT_DIR}/projects.pro" COPYONLY
	)
	FILE(REMOVE "${QT_DIR}/projects.pro.orig")
ENDIF(EXISTS "${QT_DIR}/projects.pro.orig")


TO_NATIVE_PATH("${JPEG_DIR}" JPEG_INCLUDE_DIR)
TO_NATIVE_PATH("${ZLIB_DIR}" ZLIB_INCLUDE_DIR)
TO_NATIVE_PATH("${PNG_DIR}" PNG_INCLUDE_DIR)
TO_NATIVE_PATH("${QT_DIR}" QT_DIR_NATIVE)

# Find all *.pro files under tools/ and patch those that mention qjpeg
# but not libjpeg.  Patching is necessary to make those tools compile
# with static and external libjpeg.
FILE(GLOB_RECURSE pro_files "${QT_DIR}/tools/*.pro")
FOREACH(file_pro ${pro_files})
	FILE(READ "${file_pro}" contents)
	IF(contents MATCHES qjpeg AND NOT contents MATCHES libjpeg)
		# Make a backup copy, if not already there.
		IF(NOT EXISTS "${file_pro}.orig")
			CONFIGURE_FILE("${file_pro}" "${file_pro}.orig" COPYONLY)
		ENDIF(NOT EXISTS "${file_pro}.orig")

		FILE(APPEND "${file_pro}" "\nwin32 {\n\tLIBS += libjpeg.lib\n}\n")
	ENDIF(contents MATCHES qjpeg AND NOT contents MATCHES libjpeg)
ENDFOREACH(file_pro)

# Find all *.conf files under mkspecs that mention -Zc:wchar_t- and remove
# that minus at the end.  That's necessary to make Qt compatible with other
# libraries that use wchar_t stuff.
FILE(GLOB_RECURSE conf_files "${QT_DIR}/mkspecs/*.conf")
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

SET(qt_maybe_skip_tools)
IF(EXISTS "${QT_DIR}/bin/designer.exe")
	# Either the tools have already been built, or they came prepackaged.
	SET(qt_maybe_skip_tools "-nomake tools")
ENDIF(EXISTS "${QT_DIR}/bin/designer.exe")

IF(MINGW)
	SET(platform win32-g++)
	SET(make_command mingw32-make)
ELSEIF(MSVC)
	#MESSAGE("MSVC=${MSVC} | MSVC10=${MSVC10} | MSVC80=${MSVC80}" )
	SET(make_command nmake)
	IF(MSVC60 OR MSVC70)
		MESSAGE(FATAL_ERROR "This Visual Studio version is too old and is not supported")
	ELSEIF(MSVC71)
		SET(platform win32-msvc2003)
	ELSEIF(MSVC80)
		SET(platform win32-msvc2005)
		SET(force_frame_pointer TRUE)
	ELSEIF(MSVC10)
		SET(platform win32-msvc2010)
		SET(force_frame_pointer TRUE)
	ELSE(MSVC60 OR MSVC70)
		#MSVC90 or above
		SET(platform win32-msvc2008)
		SET(force_frame_pointer TRUE)
	ENDIF(MSVC60 OR MSVC70)
ELSE(MINGW)
	MESSAGE(FATAL_ERROR "Unsupported platform.")
ENDIF(MINGW)

# We disable frame pointer ommission in release mode to ensure
# good stack traces from our built-in crash reporter.
IF(MSVC AND force_frame_pointer)
	SET(conf_file "${QT_DIR}/mkspecs/${platform}/qmake.conf")
	FILE(READ "${conf_file}" contents)
	STRING(REGEX REPLACE "(QMAKE_CFLAGS_RELEASE[ \\t]*=)" "\\1 -Oy-" new_contents "${contents}")
	STRING(REGEX REPLACE "( -Oy-)+" " -Oy-" new_contents "${new_contents}")
	IF(NOT "${contents}" STREQUAL "${new_contents}")
		# Make a backup copy, if not already there.
		IF(NOT EXISTS "${conf_file}.orig")
			CONFIGURE_FILE("${conf_file}" "${conf_file}.orig" COPYONLY)
		ENDIF()

		FILE(WRITE "${conf_file}" "${new_contents}")
	ENDIF()
ENDIF()

# Note #1: -system-zlib is broken on Windows (bug: #186774).  So, we build with
# internal zlib, while libpng is linked with an external one.  This works only
# because both zlib's have the same ABI.
# Note #2: We build with -no-libtiff because we have our own TIFF loading
# and saving code.  Qt's TIFF plugin has way too many limitations.
SET(build_qt_fname "${CMAKE_CURRENT_BINARY_DIR}/build-qt.bat")
STRING(SUBSTRING ${QT_DIR_NATIVE} 0 2 qt_drive)
STRING(SUBSTRING ${PROJECT_SOURCE_DIR} 0 2 build_drive)
FILE(
	WRITE "${build_qt_fname}"
	"${qt_drive}\n"
	"cd \"${QT_DIR_NATIVE}\"\n"
	"configure -platform ${platform} -debug-and-release -static -fast"
	" -no-gif -system-libpng -no-libmng -no-libtiff -system-libjpeg"
	" -no-openssl -no-qt3support -no-style-plastique -no-style-cleanlooks"
	" -no-style-motif -no-style-cde -no-phonon -no-webkit -no-scripttools"
	" -nomake examples -nomake demos -nomake docs ${qt_maybe_skip_tools}"
	" -opensource -confirm-license -no-ltcg"
	" -I \"${JPEG_INCLUDE_DIR}\" -I \"${ZLIB_INCLUDE_DIR}\""
	" -I \"${PNG_INCLUDE_DIR}\" -L \"${JPEG_DIR}\" -L \"${ZLIB_DIR}\""
	" -L \"${PNG_DIR}\""
	" -D _BIND_TO_CURRENT_VCLIBS_VERSION=1"
	"\n"
	"${make_command}\n"
	${build_drive}
)

ADD_CUSTOM_TARGET(Qt ALL "${build_qt_fname}")

# boost added so that boost is built before Qt. That's helpful if build
# errors occur, as rebuilding boost is much faster than rebuilding Qt.
ADD_DEPENDENCIES(Qt zlib jpeg png tiff boost)

# If you have multiple versions of Qt installed, this script
# can easily pick the wrong one.
MESSAGE(STATUS "----------------------------")
MESSAGE(STATUS "QT_DIR is ${QT_DIR}")
MESSAGE(STATUS "Check it's the one you want.")
MESSAGE(STATUS "----------------------------")
