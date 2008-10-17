CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)

PROJECT(deps)

INCLUDE(../../../cmake/SetDefaultBuildType.cmake)
INCLUDE(../../../cmake/SetDefaultGccFlags.cmake)

ST_SET_DEFAULT_BUILD_TYPE(Release)
ST_SET_DEFAULT_GCC_FLAGS()

GET_FILENAME_COMPONENT(outer_dir "${PROJECT_SOURCE_DIR}/../../../.." ABSOLUTE)

FILE(GLOB jpeg_dirs "${outer_dir}/jpeg-[0-9]*")
FILE(GLOB zlib_dirs "${outer_dir}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs "${outer_dir}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB tiff_dirs "${outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB qt_dirs "${outer_dir}/qt-win-*-4.[0-9]*")
FILE(GLOB qt_dirs2 "C:/Qt/4.*.*")

FIND_PATH(
	JPEG_DIR jpeglib.h PATHS ${jpeg_dirs}
	DOC "Path to jpeg source directory."
)
FIND_PATH(
	ZLIB_DIR zlib.h PATHS ${zlib_dirs}
	DOC "Path to zlib source directory."
)
FIND_PATH(
	PNG_DIR png.h PATHS ${png_dirs}
	DOC "Path to libpng source directory."
)
FIND_PATH(
	TIFF_DIR libtiff/tiff.h PATHS ${tiff_dirs}
	DOC "Path to top-level tiff source directory."
)
FIND_PATH(
	QT_DIR projects.pro PATHS ${qt_dirs} ${qt_dirs2}
	DOC "Path to top-level Qt source directory."
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
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c
	jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c
	jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c
	jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c
	jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c
	jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c
	jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c
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

# For some reason, Qt (as of 4.4.2) links with -lpng.lib, not just -lpng
# So, we copy libpng.a to libpng.lib.a
ADD_CUSTOM_COMMAND(
	TARGET png POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${png_location}" "${PNG_DIR}/libpng.lib.a"
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

CONFIGURE_FILE(
	"${TIFF_DIR}/libtiff/tif_config.h.vc"
	"${TIFF_DIR}/libtiff/tif_config.h" COPYONLY
)

ADD_DEFINITIONS(${tiff_definitions})
ADD_LIBRARY(tiff STATIC ${libtiff_sources})

GET_TARGET_PROPERTY(tiff_location tiff LOCATION)
ADD_CUSTOM_COMMAND(
	TARGET tiff POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${tiff_location}" "${TIFF_DIR}/libtiff/"
)


#=================================== Qt ===================================#

# Backward compatibility.  We used to patch projects.pro, now we patch
# .qmake.cache instead.
IF(EXISTS "${QT_DIR}/projects.pro.orig")
	CONFIGURE_FILE(
		"${QT_DIR}/projects.pro.orig"
		"${QT_DIR}/projects.pro" COPYONLY
	)
	FILE(REMOVE "${QT_DIR}/projects.pro.orig")
ENDIF(EXISTS "${QT_DIR}/projects.pro.orig")


# FILE(TO_NATIVE_PATH ...) is broken on MinGW.
MACRO(TO_NATIVE_PATH PATH OUT)
	FILE(TO_NATIVE_PATH "${PATH}" "${OUT}")
	STRING(REPLACE "/" "\\" "${OUT}" "${${OUT}}")
ENDMACRO(TO_NATIVE_PATH)

TO_NATIVE_PATH("${JPEG_DIR}" JPEG_INCLUDE_DIR)
TO_NATIVE_PATH("${ZLIB_DIR}" ZLIB_INCLUDE_DIR)
TO_NATIVE_PATH("${PNG_DIR}" PNG_INCLUDE_DIR)
TO_NATIVE_PATH("${QT_DIR}" QT_DIR_NATIVE)


SET(qt_build_parts "libs tools")
IF(EXISTS "${QT_DIR}/bin/designer.exe")
	# Using a binary build
	SET(qt_build_parts "libs")
ENDIF(EXISTS "${QT_DIR}/bin/designer.exe")

CONFIGURE_FILE("patch_qmake_cache.cmake.in" "${CMAKE_BINARY_DIR}/patch_qmake_cache.cmake" @ONLY) 


# Note #1: -system-zlib is broken on Windows (bug: #186774).  So, we build with
# internal zlib, while libpng is linked with an external zlib.  This works only
# because both zlib's have the same ABI.
# Note #2: We build with -no-libtiff because we have our own TIFF loading code.
# Qt's TIFF loader doesn't support multipage TIFFs or TIFFs with JPEG encoding.
SET(build_qt_fname "${CMAKE_CURRENT_BINARY_DIR}/build-qt.bat")
FILE(
	WRITE "${build_qt_fname}"
	"cd \"${QT_DIR_NATIVE}\"\n"
	"configure -release -static -fast -no-gif -system-libpng"
	" -no-libmng -no-libtiff -system-libjpeg -no-sse -no-sse2"
	" -no-openssl -no-qt3support -no-style-plastique -no-style-cleanlooks"
	" -no-style-motif -no-style-cde -no-phonon -no-webkit -no-sql-sqlite"
	" -I \"${JPEG_INCLUDE_DIR}\" -I \"${ZLIB_INCLUDE_DIR}\""
	" -I \"${PNG_INCLUDE_DIR}\" -L \"${JPEG_DIR}\" -L \"${ZLIB_DIR}\""
	" -L \"${PNG_DIR}\""
	"\n"
	"IF %ERRORLEVEL% NEQ 0 GOTO exit\n"
	"\"${CMAKE_COMMAND}\" -P \"${CMAKE_BINARY_DIR}\\patch_qmake_cache.cmake\"\n"
	"mingw32-make\n"
	":exit\n"
)

ADD_CUSTOM_TARGET(Qt ALL "${build_qt_fname}")

ADD_DEPENDENCIES(Qt zlib jpeg png tiff)
