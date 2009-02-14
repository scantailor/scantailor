CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(COMMAND cmake_policy)
	# Suppress warnings.
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy) 

PROJECT("Scan Tailor")

ENABLE_TESTING()

FILE(STRINGS "${PROJECT_SOURCE_DIR}/VERSION" VERSION LIMIT_COUNT 1)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(cmake/FindPthreads.cmake)
INCLUDE(cmake/SetDefaultBuildType.cmake)
INCLUDE(cmake/SetDefaultGccFlags.cmake)
INCLUDE(cmake/ToNativePath.cmake)

ST_SET_DEFAULT_BUILD_TYPE(Release)
IF(CMAKE_COMPILER_IS_GNUCC)
	ST_SET_DEFAULT_GCC_FLAGS()
ENDIF(CMAKE_COMPILER_IS_GNUCC)

GET_FILENAME_COMPONENT(outer_dir "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)

IF(WIN32 AND NOT QT_QMAKE_EXECUTABLE)
	FILE(GLOB qt_dirs "${outer_dir}/qt-win-*-4.[0-9]*")
	FILE(GLOB qt_dirs2 "C:/Qt/4.*.*")
	FIND_PROGRAM(
		QT_QMAKE_EXECUTABLE qmake
		PATHS dummy_dir ${qt_dirs} ${qt_dirs2}
		PATH_SUFFIXES bin
	)
ENDIF(WIN32 AND NOT QT_QMAKE_EXECUTABLE)

INCLUDE(FindQt4)

FILE(GLOB jpeg_dirs "${outer_dir}/jpeg-[0-9]*")
FILE(GLOB zlib_dirs "${outer_dir}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs "${outer_dir}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB tiff_dirs "${outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")

FIND_PATH(
	JPEG_INCLUDE_DIR jpeglib.h
	PATHS /usr/local/include /usr/include ${jpeg_dirs}
	DOC "Path to libjpeg headers."
)
FIND_LIBRARY(
	JPEG_LIBRARY jpeg
	PATHS /usr/local/lib /usr/lib ${jpeg_dirs}
	DOC "Path to jpeg library."
)
INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")

FIND_PATH(
	ZLIB_INCLUDE_DIR zlib.h
	PATHS /usr/local/include /usr/include ${zlib_dirs}
	DOC "Path to zlib headers."
)
FIND_LIBRARY(
	ZLIB_LIBRARY NAMES zlib z
	PATHS /usr/local/lib /usr/lib ${zlib_dirs}
	DOC "Path to zlib library."
)
INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

FIND_PATH(
	PNG_INCLUDE_DIR png.h
	PATHS /usr/local/include /usr/include ${png_dirs}
	DOC "Path to libpng headers."
)
FIND_LIBRARY(
	PNG_LIBRARY png
	PATHS /usr/local/lib /usr/lib ${png_dirs}
	DOC "PAth to png library."
)
INCLUDE_DIRECTORIES("${PNG_INCLUDE_DIR}")

FIND_PATH(
	TIFF_INCLUDE_DIR tiff.h
	PATHS /usr/local/include /usr/include ${tiff_dirs}
	PATH_SUFFIXES libtiff
	DOC "Path to libtiff headers."
)
FIND_LIBRARY(
	TIFF_LIBRARY tiff
	PATHS /usr/local/lib /usr/lib ${tiff_dirs}
	PATH_SUFFIXES libtiff
	DOC "Path to tiff library."
)
INCLUDE_DIRECTORIES("${TIFF_INCLUDE_DIR}")

IF(UNIX)
	# We need to manually use XRender.
	FIND_PATH(
		X11_INCLUDE_PATH extensions/Xrender.h
		PATHS /usr/local /usr
		PATH_SUFFIXES include/X11 X11/include
		DOC "The path to extensions/Xrender.h."
	)
	IF (NOT X11_INCLUDE_PATH)
		MESSAGE(
			FATAL_ERROR
			"Could not find Xrender.h\n"
			"Most likely you need to install a package called\n"
			"libxrender-dev or libXrender-dev or similar."
		)
	ENDIF(NOT X11_INCLUDE_PATH)
	
	INCLUDE_DIRECTORIES("${X11_INCLUDE_PATH}")
	
	FindPthreads()
	IF(PTHREADS_FOUND)
		ADD_DEFINITIONS(${PTHREADS_CFLAGS})
		LINK_LIBRARIES(${PTHREADS_LIBS})
	ELSE(PTHREADS_FOUND)
		MESSAGE(
			FATAL_ERROR
			"Could not detect threading flags.\n"
			"Try specifying them manually in PTHREADS_CFLAGS and PTHREADS_LIBS."
		)
	ENDIF(PTHREADS_FOUND)
ELSEIF(MINGW)
	ADD_DEFINITIONS(-mthreads)
	LINK_LIBRARIES(-mthreads)
ENDIF(UNIX)

CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
IF(NOT HAVE_STDINT_H)
	CONFIGURE_FILE(compat/pstdint.h "${CMAKE_CURRENT_BINARY_DIR}/stdint.h" COPYONLY)
	INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
ELSE(NOT HAVE_STDINT_H)
	FILE(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/stdint.h")
ENDIF(NOT HAVE_STDINT_H)

ADD_DEFINITIONS(
	${QT_DEFINITIONS} -DBOOST_SIGNALS_NAMESPACE=signals
	-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION
)

IF(WIN32)
	ADD_DEFINITIONS(-DLIBTIFF_STATIC)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(. foundation boost ${QT_INCLUDES})
LINK_DIRECTORIES(${QT_LIBRARY_DIR})


SET(EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})
IF(WIN32)
	LINK_DIRECTORIES("${QT_PLUGINS_DIR}/imageformats")
	LIST(
		APPEND EXTRA_LIBS
		${QT_QTMAIN_LIBRARY} qjpeg
		winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool
	)
ENDIF(WIN32)

# Prepare config.h
IF(WIN32)
	SET(TRANSLATIONS_DIR_REL "translations")
ELSE(WIN32)
	SET(TRANSLATIONS_DIR_REL "share/scantailor/translations")
ENDIF(WIN32)
SET(TRANSLATIONS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/${TRANSLATIONS_DIR_REL}")
CONFIGURE_FILE(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)


ADD_SUBDIRECTORY(boost)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(tests)

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("Ui Files" FILES ${ui_files})
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

ADD_SUBDIRECTORY(filters/fix_orientation)
ADD_SUBDIRECTORY(filters/page_split)
ADD_SUBDIRECTORY(filters/deskew)
ADD_SUBDIRECTORY(filters/select_content)
ADD_SUBDIRECTORY(filters/page_layout)
ADD_SUBDIRECTORY(filters/output)

SET(resource_sources)
QT4_ADD_RESOURCES(resource_sources resources/resources.qrc)
SOURCE_GROUP("Generated Sources" FILES ${ui_sources} ${resource_sources})

SET(
	sources
	main.cpp Application.cpp Application.h
	BackgroundExecutor.cpp BackgroundExecutor.h
	PixmapRenderer.cpp PixmapRenderer.h
	Dpi.cpp Dpi.h Dpm.cpp Dpm.h
	SmartFilenameOrdering.cpp SmartFilenameOrdering.h
	ImageInfo.cpp ImageInfo.h
	ImageFileInfo.cpp ImageFileInfo.h
	ImageMetadata.cpp ImageMetadata.h
	PageLayout.cpp PageLayout.h
	RecentProjects.cpp RecentProjects.h
	FixDpiDialog.cpp FixDpiDialog.h
	ProjectCreationContext.cpp ProjectCreationContext.h
	MainWindow.cpp MainWindow.h
	ContentSpanFinder.cpp ContentSpanFinder.h
	PhysicalTransformation.cpp PhysicalTransformation.h
	ImageTransformation.cpp ImageTransformation.h
	ImageViewBase.cpp ImageViewBase.h
	BasicImageView.cpp BasicImageView.h
	ThumbnailLoadResult.h
	ThumbnailPixmapCache.cpp ThumbnailPixmapCache.h
	ThumbnailBase.cpp ThumbnailBase.h
	ThumbnailSequence.cpp ThumbnailSequence.h
	ThumbnailFactory.cpp ThumbnailFactory.h
	IncompleteThumbnail.cpp IncompleteThumbnail.h
	ContentBoxPropagator.cpp ContentBoxPropagator.h
	DebugImages.cpp DebugImages.h
	ImageId.cpp ImageId.h
	PageId.cpp PageId.h
	PageInfo.cpp PageInfo.h
	BackgroundTask.cpp BackgroundTask.h
	PageSequence.cpp PageSequence.h
	FilterData.cpp FilterData.h
	ImageMetadataLoader.cpp ImageMetadataLoader.h
	TiffReader.cpp TiffReader.h
	TiffWriter.cpp TiffWriter.h
	PngMetadataLoader.cpp PngMetadataLoader.h
	TiffMetadataLoader.cpp TiffMetadataLoader.h
	JpegMetadataLoader.cpp JpegMetadataLoader.h
	ImageLoader.cpp ImageLoader.h
	ErrorWidget.cpp ErrorWidget.h
	OrthogonalRotation.cpp OrthogonalRotation.h
	NewOpenProjectPanel.cpp NewOpenProjectPanel.h
	ProjectFilesDialog.cpp ProjectFilesDialog.h
	WorkerThread.cpp WorkerThread.h
	LoadFileTask.cpp LoadFileTask.h
	FilterOptionsWidget.cpp FilterOptionsWidget.h
	TaskStatus.h FilterUiInterface.h
	ProjectReader.cpp ProjectReader.h
	ProjectWriter.cpp ProjectWriter.h
	XmlMarshaller.cpp XmlMarshaller.h
	XmlUnmarshaller.cpp XmlUnmarshaller.h
	AtomicFileOverwriter.cpp AtomicFileOverwriter.h
	EstimateBackground.cpp EstimateBackground.h
	Despeckle.cpp Despeckle.h
	Utils.cpp Utils.h
	AutoManualMode.h
	AbstractCommand.h
	AbstractFilter.h
	FilterResult.h
	CompositeCacheDrivenTask.h
	Margins.h
	filter_dc/AbstractFilterDataCollector.h
	filter_dc/ThumbnailCollector.h
	filter_dc/ContentBoxCollector.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_EXECUTABLE(scantailor WIN32 ${sources} ${ui_sources} ${resource_sources})
TARGET_LINK_LIBRARIES(
	scantailor
	fix_orientation page_split deskew select_content page_layout output
	imageproc foundation
	${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)
INSTALL(TARGETS scantailor RUNTIME DESTINATION bin)

MACRO(CREATE_UPDATE_TRANSLATIONS_TARGET _sources_ _ts_files_)
	SET(_sources)
	SET(_ts_files)
	FOREACH(_file ${ARGV})
		GET_FILENAME_COMPONENT(_abs "${_file}" ABSOLUTE)
		GET_FILENAME_COMPONENT(_ext "${_file}" EXT)
		IF(_ext STREQUAL .ts)
			LIST(APPEND _ts_files "${_abs}")
		ELSE(_ext STREQUAL .ts)
			LIST(APPEND _sources "${_abs}")
		ENDIF(_ext STREQUAL .ts)
	ENDFOREACH(_file)
	
	ADD_CUSTOM_TARGET(
		update_translations
		COMMAND "${QT_LUPDATE_EXECUTABLE}"
		${_sources} -ts ${_ts_files}
		DEPENDS ${_sources}
	)
ENDMACRO(CREATE_UPDATE_TRANSLATIONS_TARGET)

# Translations
FILE(GLOB abs_sources ABSOLUTE ${sources})
FILE(GLOB abs_ui_files ABSOLUTE ${ui_files})
SET(TRANSLATION_SOURCES ${TRANSLATION_SOURCES} ${abs_sources} ${abs_ui_files})
FILE(GLOB TRANSLATION_FILES translations/*.ts)

CREATE_UPDATE_TRANSLATIONS_TARGET(${TRANSLATION_SOURCES} ${TRANSLATION_FILES})

# Remove untranslated.ts from TRANSLATION_FILES, as we don't want
# untranslated.qm to be built.
SET(FILTERED_TRANSLATION_FILES)
FOREACH(ts_file ${TRANSLATION_FILES})
	IF ("${ts_file}" MATCHES ".*untranslated.ts")
		# Just skip it.
	ELSE("${ts_file}" MATCHES ".*untranslated.ts")
		LIST(APPEND FILTERED_TRANSLATION_FILES "${ts_file}")
	ENDIF("${ts_file}" MATCHES ".*untranslated.ts")
ENDFOREACH(ts_file)
QT4_ADD_TRANSLATION(QM_FILES ${FILTERED_TRANSLATION_FILES})
ADD_CUSTOM_TARGET(compile_translations ALL DEPENDS ${QM_FILES})

INSTALL(FILES ${QM_FILES} DESTINATION "${TRANSLATIONS_DIR_REL}/")

# Tests
ADD_TEST(ImageProcTests imageproc/tests/imageproc_tests --log_level=message)
ADD_TEST(ScanTaylorTests tests/tests --log_level=message)

# Source code packaging
SET(CPACK_CMAKE_GENERATOR "")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "scantailor-${VERSION}")
SET(
	CPACK_SOURCE_IGNORE_FILES
	"/\\\\.svn/"
	"~$"
	"\\\\.pcs$"
	"TODO.txt"
	"/doxygen/"
	"${CMAKE_BINARY_DIR}"
)
INCLUDE(CPack)

# Installer for Windows.
IF(WIN32)
	ADD_SUBDIRECTORY(packaging/windows)
ENDIF(WIN32)
