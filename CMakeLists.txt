CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

IF(COMMAND cmake_policy)
	# Suppress warnings.
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy) 

PROJECT("Scan Tailor")

# This forces Visual Studio 2008 SP1 to actually link to the versions of the
# libraries it ships with!  It's harmless on other platforms.
ADD_DEFINITIONS(-D_BIND_TO_CURRENT_VCLIBS_VERSION=1)

ENABLE_TESTING()

# A hack to force cmake to be re-run when the VERSION file changes.
CONFIGURE_FILE(
	"${PROJECT_SOURCE_DIR}/VERSION"
	"${PROJECT_BINARY_DIR}/VERSION" COPYONLY
)

FILE(STRINGS "${PROJECT_SOURCE_DIR}/VERSION" VERSION LIMIT_COUNT 1)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(FindBoost)
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
IF(NOT QT4_FOUND)
	MESSAGE(
		FATAL_ERROR "Qt4 could not be found. "
		"If it's installed in a non-standard location, specify the path to qmake in QT_QMAKE_EXECUTABLE. "
		"You can do it in interactive mode (ccmake instead of cmake) or using -DVAR=VAL syntax."
	)
ENDIF(NOT QT4_FOUND)

FILE(GLOB jpeg_dirs "${outer_dir}/jpeg-[0-9]*")
FILE(GLOB zlib_dirs "${outer_dir}/zlib-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB png_dirs "${outer_dir}/libpng-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB tiff_dirs "${outer_dir}/tiff-[0-9]*.[0-9]*.[0-9]*")
FILE(GLOB boost_dirs "${outer_dir}/boost_1_[0-9]*_[0-9]*")

FIND_PATH(
	JPEG_INCLUDE_DIR jpeglib.h
	PATHS /usr/local/include /usr/include ${jpeg_dirs}
	DOC "Path to libjpeg headers."
)
IF(NOT JPEG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find jpeg headers.\n"
		"You may need to install a package named libjpeg62-dev or similarly."
	)
ENDIF(NOT JPEG_INCLUDE_DIR)

FIND_LIBRARY(
	JPEG_LIBRARY NAMES jpeg libjpeg.lib
	PATHS /usr/local/lib /usr/lib ${jpeg_dirs}
	DOC "Path to jpeg library."
)
INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")

FIND_PATH(
	ZLIB_INCLUDE_DIR zlib.h
	PATHS /usr/local/include /usr/include ${zlib_dirs}
	DOC "Path to zlib headers."
)
IF(NOT ZLIB_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find zlib headers.\n"
		"You may need to install a package named zlib1g-dev or similarly."
	)
ENDIF(NOT ZLIB_INCLUDE_DIR)

FIND_LIBRARY(
	ZLIB_LIBRARY NAMES z libz.lib
	PATHS /usr/local/lib /usr/lib ${zlib_dirs}
	DOC "Path to zlib library."
)
INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

FIND_PATH(
	PNG_INCLUDE_DIR png.h
	PATHS /usr/local/include /usr/include ${png_dirs}
	DOC "Path to libpng headers."
)
IF(NOT PNG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find libpng headers.\n"
		"You may need to install a package named libpng12-dev or similarly."
	)
ENDIF(NOT PNG_INCLUDE_DIR)

FIND_LIBRARY(
	PNG_LIBRARY NAMES png libpng.lib
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
IF(NOT TIFF_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find libtiff headers.\n"
 		"You may need to install a package named libtiff4-dev or similarly."
	)
ENDIF(NOT TIFF_INCLUDE_DIR)

FIND_LIBRARY(
	TIFF_LIBRARY tiff libtiff.lib
	PATHS /usr/local/lib /usr/lib ${tiff_dirs}
	PATH_SUFFIXES libtiff
	DOC "Path to tiff library."
)
INCLUDE_DIRECTORIES("${TIFF_INCLUDE_DIR}")

IF(WIN32)
	FIND_PATH(
		BOOST_ROOT boost-build.jam PATHS ${boost_dirs}
		DOC "Path to top-level Boost source directory."
	)
	SET(Boost_USE_STATIC_LIBS ON)
ELSE(WIN32)
	ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ENDIF(WIN32)

SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost 1.35.0 COMPONENTS unit_test_framework prg_exec_monitor)
IF(NOT Boost_FOUND)
	MESSAGE(
		FATAL_ERROR
		"Could not find boost headers or libraries.\n"
		"You may need to install a package named libboost1.35-dev or similarly."
	)
ENDIF(NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

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
			"You may need to install a package called\n"
			"libxrender-dev or libXrender-dev or similarly."
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
ELSEIF(WIN32)
	ADD_DEFINITIONS(-DNOMINMAX)
ENDIF(UNIX)

CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
IF(NOT HAVE_STDINT_H)
	CONFIGURE_FILE(compat/pstdint.h "${CMAKE_CURRENT_BINARY_DIR}/stdint.h" COPYONLY)
	INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
ELSE(NOT HAVE_STDINT_H)
	FILE(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/stdint.h")
ENDIF(NOT HAVE_STDINT_H)

ADD_DEFINITIONS(
	${QT_DEFINITIONS} -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION
)

IF(WIN32)
 	ADD_DEFINITIONS(-DLIBTIFF_STATIC)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(. foundation ${QT_INCLUDES})
LINK_DIRECTORIES(${QT_LIBRARY_DIR})

SET(EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})
IF(WIN32)
	FIND_LIBRARY(QJPEG_RELEASE qjpeg PATHS "${QT_PLUGINS_DIR}/imageformats")
	FIND_LIBRARY(QJPEG_DEBUG qjpegd PATHS "${QT_PLUGINS_DIR}/imageformats")
	MARK_AS_ADVANCED(FORCE QJPEG_RELEASE QJPEG_DEBUG)
	IF(QJPEG_RELEASE AND NOT QJPEG_DEBUG)
		SET(QJPEG_DEBUG "${QJPEG_RELEASE}")
	ELSEIF(QJPEG_DEBUG AND NOT QJPEG_RELEASE)
		SET(QJPEG_RELEASE "${QJPEG_DEBUG}")
	ENDIF(QJPEG_RELEASE AND NOT QJPEG_DEBUG)
	LIST(
		APPEND EXTRA_LIBS
		"${QT_QTMAIN_LIBRARY}" optimized "${QJPEG_RELEASE}"
		debug "${QJPEG_DEBUG}"
		winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool
	)
ENDIF(WIN32)

# Crash reporter, only Windows + MSVC supported for now.
# Must go before generating config.h, as ENABLE_CRASH_REPORTER
# goes in there.
IF(MSVC)
	OPTION(ENABLE_CRASH_REPORTER "Enable crash reporter (only for official builds)" OFF)
ENDIF(MSVC)

# Prepare config.h
IF(WIN32)
	SET(TRANSLATIONS_DIR_REL "translations")
ELSE(WIN32)
	SET(TRANSLATIONS_DIR_REL "share/scantailor/translations")
ENDIF(WIN32)
SET(TRANSLATIONS_DIR_ABS "${CMAKE_INSTALL_PREFIX}/${TRANSLATIONS_DIR_REL}")

CONFIGURE_FILE(config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)
CONFIGURE_FILE(version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

# crash_reporter is included unconditionally to collect translation sources from there.
ADD_SUBDIRECTORY(crash_reporter)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(tests)

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("Ui Files" FILES ${ui_files})
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

ADD_CUSTOM_TARGET(toplevel_ui_sources DEPENDS ${ui_sources})

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
	SkinnedButton.cpp SkinnedButton.h
	BubbleAnimation.cpp BubbleAnimation.h
	ProcessingIndicationWidget.cpp ProcessingIndicationWidget.h
	Dpi.cpp Dpi.h Dpm.cpp Dpm.h
	SmartFilenameOrdering.cpp SmartFilenameOrdering.h
	ImageInfo.cpp ImageInfo.h
	ImageFileInfo.cpp ImageFileInfo.h
	ImageMetadata.cpp ImageMetadata.h
	PageLayout.cpp PageLayout.h
	RecentProjects.cpp RecentProjects.h
	FixDpiDialog.cpp FixDpiDialog.h
	FixDpiSinglePageDialog.cpp FixDpiSinglePageDialog.h
	ProjectCreationContext.cpp ProjectCreationContext.h
	ProjectOpeningContext.cpp ProjectOpeningContext.h
	MainWindow.cpp MainWindow.h
	PageSelectionAccessor.cpp PageSelectionAccessor.h
	ContentSpanFinder.cpp ContentSpanFinder.h
	PhysicalTransformation.cpp PhysicalTransformation.h
	ImageTransformation.cpp ImageTransformation.h
	ImageViewBase.cpp ImageViewBase.h
	BasicImageView.cpp BasicImageView.h
	StageListView.cpp StageListView.h
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
	StageSequence.cpp StageSequence.h
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
	Zone.cpp Zone.h
	AutoManualMode.h
	AbstractCommand.h
	AbstractFilter.h
	BeforeOrAfter.h
	FilterResult.h
	CompositeCacheDrivenTask.h
	Margins.h
	ChangedStateItemDelegate.h
	PageRange.h
	filter_dc/AbstractFilterDataCollector.h
	filter_dc/ThumbnailCollector.h
	filter_dc/ContentBoxCollector.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

SET(win32_resource_file)
IF(WIN32)
	SET(rc_file "${CMAKE_SOURCE_DIR}/resources/win32/resources.rc")
	FILE(GLOB win32_resources resources/win32/*.ico)
	SET_SOURCE_FILES_PROPERTIES(
		"${rc_file}" PROPERTIES
		OBJECT_DEPENDS ${win32_resources}
	)
	IF(MINGW)
		# CMake doesn't know how to process .rc files with MinGW.
		SET(win32_resource_file "${CMAKE_BINARY_DIR}/win32_resources.o")
		ADD_CUSTOM_COMMAND(
			OUTPUT "${win32_resource_file}"
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resources/win32"
			COMMAND windres -i "${rc_file}" -o "${win32_resource_file}"
			MAIN_DEPENDENCY "${rc_file}"
			DEPENDS ${win32_resources}
		)
	ELSE(MINGW)
		SET(win32_resource_file "${rc_file}")
	ENDIF(MINGW)
ENDIF(WIN32)

ADD_EXECUTABLE(
	scantailor WIN32 ${sources} ${ui_sources}
	${resource_sources} ${win32_resource_file}
)
TARGET_LINK_LIBRARIES(
	scantailor
	fix_orientation page_split deskew select_content page_layout output
	imageproc foundation
	${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)
INSTALL(TARGETS scantailor RUNTIME DESTINATION bin)
GET_TARGET_PROPERTY(scantailor_executable scantailor LOCATION)

IF(ENABLE_CRASH_REPORTER)
	FIND_PATH(
		SYMBOLS_PATH . PATHS "${outer_dir}/symbols" NO_DEFAULT_PATH
		DOC "The directory to write symbol information into."
	)
	IF(NOT SYMBOLS_PATH)
		MESSAGE(FATAL_ERROR "SYMBOLS_PATH directory is not set.")
	ENDIF(NOT SYMBOLS_PATH)

	# We can't build it, because it requires ATL, which is not part
	# of the Visual Studio Express Edition, so we rely on a pre-built
	# version which can be found in the Mozilla repository.
	FIND_PROGRAM(
		DUMP_SYMS_EXECUTABLE dump_syms PATHS "${outer_dir}"
		DOC "Path to dump_syms.exe, which can be found in Mozilla repository."
	)
	IF(NOT DUMP_SYMS_EXECUTABLE)
		MESSAGE(
			FATAL_ERROR "dump_syms.exe wasn't found.  Specify its location manually by setting the DUMP_SYMS_EXECUTABLE variable.  dump_syms.exe may be found in the Mozilla repository."
		)
	ENDIF(NOT DUMP_SYMS_EXECUTABLE)
	
	INCLUDE_DIRECTORIES(crash_reporter crash_reporter/google-breakpad)
	TARGET_LINK_LIBRARIES(scantailor crash_handler)
	
	ADD_CUSTOM_COMMAND(
		TARGET scantailor POST_BUILD
		COMMAND "${CMAKE_COMMAND}" ARGS -E echo "Dumping symbols..."
        COMMAND "${DUMP_SYMS_EXECUTABLE}" ARGS "${scantailor_executable}" ">" "temp.sym"
		COMMAND "${CMAKE_COMMAND}" ARGS -P "${CMAKE_SOURCE_DIR}/cmake/move_sym_file.cmake"
		WORKING_DIRECTORY "${SYMBOLS_PATH}"
	)
ENDIF(ENABLE_CRASH_REPORTER)

SET(update_translations_subtargets)
MACRO(UPDATE_TRANSLATIONS _subtarget) #, sources, ts_files
	SET(_sources_str "")
	SET(_translations_str "")
	FOREACH(_file ${ARGN})
		GET_FILENAME_COMPONENT(_abs "${_file}" ABSOLUTE)
		GET_FILENAME_COMPONENT(_ext "${_file}" EXT)
		IF(_ext STREQUAL .ts)
			SET(_translations_str "${_translations_str} \"${_abs}\"")
		ELSE(_ext STREQUAL .ts)
			SET(_sources_str "${_sources_str} \"${_abs}\"")
		ENDIF(_ext STREQUAL .ts)
	ENDFOREACH(_file)
	
	FILE(
		WRITE "${CMAKE_BINARY_DIR}/update_translations_${_subtarget}.pro"
		"SOURCES = ${_sources_str}\nTRANSLATIONS = ${_translations_str}"
	)
	
	LIST(APPEND update_translations_subtarget ${_subtarget})
ENDMACRO(UPDATE_TRANSLATIONS)

MACRO(CREATE_UPDATE_TRANSLATIONS_TARGET)
	SET(_commands)
	FOREACH(_subtarget ${update_translations_subtarget})
		LIST(
			APPEND _commands COMMAND "${QT_LUPDATE_EXECUTABLE}"
			"${CMAKE_BINARY_DIR}/update_translations_${_subtarget}.pro"
		)
	ENDFOREACH(_subtarget)
	ADD_CUSTOM_TARGET(update_translations ${_commands})
ENDMACRO(CREATE_UPDATE_TRANSLATIONS_TARGET)

# Translations
FILE(GLOB abs_sources ABSOLUTE ${sources})
FILE(GLOB abs_ui_files ABSOLUTE ${ui_files})
SET(TRANSLATION_SOURCES ${TRANSLATION_SOURCES} ${abs_sources} ${abs_ui_files})
FILE(GLOB TRANSLATION_FILES translations/scantailor_*.ts)
FILE(GLOB CR_TRANSLATION_FILES translations/crashreporter_*.ts)

UPDATE_TRANSLATIONS(scantailor ${TRANSLATION_SOURCES} ${TRANSLATION_FILES})
UPDATE_TRANSLATIONS(crashreporter ${CR_TRANSLATION_SOURCES} ${CR_TRANSLATION_FILES})
CREATE_UPDATE_TRANSLATIONS_TARGET()

SET(ts_files ${TRANSLATION_FILES})
IF(ENABLE_CRASH_REPORTER)
	LIST(APPEND ts_files ${CR_TRANSLATION_FILES})
ENDIF(ENABLE_CRASH_REPORTER)

# Don't build *.qm files from *untranslated.ts
SET(FILTERED_TRANSLATION_FILES)
FOREACH(ts_file ${ts_files})
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
