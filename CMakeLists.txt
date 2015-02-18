IF(WIN32)
	CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)
ELSE()
	CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)
ENDIF()

IF(COMMAND cmake_policy)
	# Suppress warnings.
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE
	"${CMAKE_SOURCE_DIR}/cmake/default_cflags.cmake"
)
SET(
	CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
	"${CMAKE_SOURCE_DIR}/cmake/default_cxxflags.cmake"
)

PROJECT("Scan Tailor")

# This forces Visual Studio 2008 SP1 to actually link to the versions of the
# libraries it ships with!  It's harmless on other platforms.
ADD_DEFINITIONS(-D_BIND_TO_CURRENT_VCLIBS_VERSION=1)

IF(DEBUG_CLI)
	ADD_DEFINITIONS(-DDEBUG_CLI)
ENDIF(DEBUG_CLI)

ENABLE_TESTING()

# An undocumented side-effect of CONFIGURE_FILE() is that it makes
# the whole project depend on the file we are parsing / copying.
CONFIGURE_FILE(
	"${PROJECT_SOURCE_DIR}/version.h"
	"${PROJECT_BINARY_DIR}/.version.h" COPYONLY
)

# Prevent this leftover from old builds to be used in favour
# of the one in ${PROJECT_SOURCE_DIR}
IF(NOT "${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	FILE(REMOVE "${PROJECT_BINARY_DIR}/version.h")
ENDIF()

# Extract VERSION and VERSION_QUAD from version.h
FILE(READ "${PROJECT_SOURCE_DIR}/version.h" version_h_contents)
STRING(
	REGEX REPLACE
	".*#define[ \\t]+VERSION[ \\t]+\"([^\"]*)\".*"
	"\\1" VERSION "${version_h_contents}"
)
IF("${VERSION}" STREQUAL "${version_h_contents}")
	MESSAGE(FATAL_ERROR "Failed to extract VERSION from version.h")
ENDIF()

# VERSION_QUAD must be either empty or be in the form of X.Y.Z.Y
STRING(
	REGEX REPLACE
	".*#define[ \\t]+VERSION_QUAD[ \\t]+\"(([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)?)\".*"
	"\\1" VERSION_QUAD "${version_h_contents}"
)
IF("${VERSION_QUAD}" STREQUAL "${version_h_contents}")
	MESSAGE(FATAL_ERROR "Failed to extract VERSION_QUAD from version.h")
ENDIF()

# This has to go quite early on, as otherwise we risk picking
# up an identically named header from a system include path.
INCLUDE_DIRECTORIES(. foundation math interaction zones)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(TestCXXAcceptsFlag)
INCLUDE(FindBoost)
INCLUDE(cmake/FindPthreads.cmake)
INCLUDE(cmake/SetDefaultBuildType.cmake)
INCLUDE(cmake/SetDefaultGccFlags.cmake)
INCLUDE(cmake/ToNativePath.cmake)
INCLUDE(cmake/UpdateTranslations.cmake)
INCLUDE(cmake/CopyToBuildDir.cmake)
INCLUDE(cmake/LibToDLL.cmake)

ST_SET_DEFAULT_BUILD_TYPE(Release)
IF(CMAKE_COMPILER_IS_GNUCC)
	ST_SET_DEFAULT_GCC_FLAGS()
ENDIF(CMAKE_COMPILER_IS_GNUCC)

GET_FILENAME_COMPONENT(source_outer_dir "${PROJECT_SOURCE_DIR}/../../../.." ABSOLUTE)
GET_FILENAME_COMPONENT(build_outer_dir "${PROJECT_BINARY_DIR}/.." ABSOLUTE)

SET(STAGING_LIBS_DIR "")
IF(WIN32)
	FIND_PATH(
		DEPS_BUILD_DIR build-qt.bat
		HINTS "${build_outer_dir}/scantailor-deps-build"
		DOC "Build directory for Scan Tailor dependencies."
	)
	IF(NOT DEPS_BUILD_DIR)
		MESSAGE(
			FATAL_ERROR "The build directory for Scan Tailor dependencies could not be found! "
			"You can specify it manually in DEPS_BUILD_DIR variable. Make sure you build the dependencies first!"
		)
	ENDIF()
	SET(STAGING_LIBS_DIR "${DEPS_BUILD_DIR}/staging/libs")
	
	IF (NOT EXISTS "${QT_QMAKE_EXECUTABLE}")
		FILE(GLOB qt_dirs1 "${build_outer_dir}/qt-win-*-4.[0-9]*")
		FILE(GLOB qt_dirs2 "${build_outer_dir}/qt-everywhere-*-4.[0-9]*")
		FILE(GLOB qt_dirs3 "${source_outer_dir}/qt-win-*-4.[0-9]*")
		FILE(GLOB qt_dirs4 "${source_outer_dir}/qt-everywhere-*-4.[0-9]*")
		FILE(GLOB qt_dirs5 "C:/Qt/4.*.*")
		FILE(GLOB qt_dirs6 "C:/Qt/20*/qt")
		FIND_PROGRAM(
			QT_QMAKE_EXECUTABLE qmake
			PATHS dummy_dir ${qt_dirs1} ${qt_dirs2} ${qt_dirs3}
			${qt_dirs4} ${qt_dirs5} ${qt_dirs6}
			PATH_SUFFIXES bin
		)
	ENDIF()
ENDIF()

INCLUDE(FindQt4)
IF(NOT QT4_FOUND)
	MESSAGE(
		FATAL_ERROR "Qt4 could not be found. "
		"If it's installed in a non-standard location, specify the path to qmake in QT_QMAKE_EXECUTABLE. "
		"You can do it in interactive mode (ccmake instead of cmake) or using -DVAR=VAL syntax."
	)
ENDIF(NOT QT4_FOUND)
GET_FILENAME_COMPONENT(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" PATH)


INCLUDE(FindOpenGL)
SET(use_opengl OFF)
IF(OPENGL_FOUND AND QT_QTOPENGL_FOUND)
	# Disabled for now due to various problems associated with it.
	# SET(use_opengl ON)
ENDIF()
OPTION(ENABLE_OPENGL "OpenGL may be used for UI acceleration" ${use_opengl})


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
FILE(GLOB boost_dirs1 "${build_outer_dir}/boost_1_[0-9]*_[0-9]*")
FILE(GLOB boost_dirs2 "${source_outer_dir}/boost_1_[0-9]*_[0-9]*")

FIND_PATH(
	JPEG_INCLUDE_DIR jpeglib.h
	PATHS /usr/local/include /usr/include
	HINTS ${jpeg_dirs1} ${jpeg_dirs2}
	DOC "Path to libjpeg headers."
)
IF(NOT JPEG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find jpeg headers.\n"
		"You may need to install a package named libjpeg62-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")

FIND_LIBRARY(
	JPEG_LIBRARY NAMES jpeg libjpeg.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${jpeg_dirs1} ${jpeg_dirs2} ${STAGING_LIBS_DIR}
	DOC "Path to jpeg library."
)
IF(NOT JPEG_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find jpeg library.\n"
		"You may need to install a package named libjpeg62-dev or similarly."
	)
ENDIF()


FIND_PATH(
	ZLIB_INCLUDE_DIR zlib.h
	PATHS /usr/local/include /usr/include
	HINTS ${zlib_dirs1} ${zlib_dirs2}
	DOC "Path to zlib headers."
)
IF(NOT ZLIB_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find zlib headers.\n"
		"You may need to install a package named zlib1g-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${ZLIB_INCLUDE_DIR}")

FIND_LIBRARY(
	ZLIB_LIBRARY NAMES z zdll.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${zlib_dirs1} ${zlib_dirs2} ${STAGING_LIBS_DIR}
	DOC "Path to zlib library."
)
IF(NOT ZLIB_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find zlib library.\n"
		"You may need to install a package named zlib1g-dev or similarly."
	)
ENDIF()

FIND_PATH(
	PNG_INCLUDE_DIR png.h
	PATHS /usr/local/include /usr/include
	HINTS ${png_dirs1} ${png_dirs2} ${png_dirs3} ${png_dirs4}
	DOC "Path to libpng headers."
)
IF(NOT PNG_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find libpng headers.\n"
		"You may need to install a package named libpng12-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${PNG_INCLUDE_DIR}")

FIND_LIBRARY(
	PNG_LIBRARY NAMES png libpng.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${png_dirs1} ${png_dirs2} ${png_dirs3} ${png_dirs4} ${STAGING_LIBS_DIR}
	DOC "Path to png library."
)
IF(NOT PNG_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find libpng library.\n"
		"You may need to install a package named libpng12-dev or similarly."
	)
ENDIF()


FIND_PATH(
	TIFF_INCLUDE_DIR tiff.h
	PATHS /usr/local/include /usr/include
	HINTS ${tiff_dirs1} ${tiff_dirs2}
	PATH_SUFFIXES libtiff
	DOC "Path to libtiff headers."
)
IF(NOT TIFF_INCLUDE_DIR)
	MESSAGE(
		FATAL_ERROR
		"Could not find libtiff headers.\n"
		"You may need to install a package named libtiff4-dev or similarly."
	)
ENDIF()

INCLUDE_DIRECTORIES("${TIFF_INCLUDE_DIR}")

FIND_LIBRARY(
	TIFF_LIBRARY tiff libtiff.lib
	PATHS /usr/local/lib /usr/lib
	HINTS ${tiff_dirs1} ${tiff_dirs2} ${STAGING_LIBS_DIR}
	PATH_SUFFIXES libtiff
	DOC "Path to tiff library."
)
IF(NOT TIFF_LIBRARY)
	MESSAGE(
		FATAL_ERROR
		"Could not find libtiff library.\n"
		"You may need to install a package named libtiff4-dev or similarly."
	)
ENDIF()

IF(WIN32)
	ADD_DEFINITIONS(-DUSE_LIBTIFF_DLL)
ENDIF()


IF(WIN32)
	FIND_PATH(
		BOOST_ROOT boost-build.jam PATHS ${boost_dirs1} ${boost_dirs2}
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

SET(EXTRA_LIBS "")

IF(UNIX)
	# We need to manually use XRender.
	INCLUDE(FindX11)
	IF(NOT X11_Xrender_FOUND)
		MESSAGE(
			FATAL_ERROR
			"Could not find Xrender.h\n"
			"You may need to install a package called\n"
			"libxrender-dev or libXrender-dev or similarly."
		)
	ENDIF()

	INCLUDE_DIRECTORIES("${X11_Xrender_INCLUDE_PATH}")
	LIST(APPEND EXTRA_LIBS ${X11_Xrender_LIB})

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

INCLUDE_DIRECTORIES(${QT_INCLUDES})
LINK_DIRECTORIES(${QT_LIBRARY_DIR})

#SET(QJPEG_LIBRARIES "")
IF(WIN32)
#	FIND_LIBRARY(QJPEG_RELEASE qjpeg PATHS "${QT_PLUGINS_DIR}/imageformats")
#	FIND_LIBRARY(QJPEG_DEBUG qjpegd PATHS "${QT_PLUGINS_DIR}/imageformats")
#	MARK_AS_ADVANCED(FORCE QJPEG_RELEASE QJPEG_DEBUG)
#	IF(QJPEG_RELEASE AND NOT QJPEG_DEBUG)
#		SET(QJPEG_DEBUG "${QJPEG_RELEASE}")
#	ELSEIF(QJPEG_DEBUG AND NOT QJPEG_RELEASE)
#		SET(QJPEG_RELEASE "${QJPEG_DEBUG}")
#	ELSEIF(NOT QJPEG_DEBUG AND NOT QJPEG_RELEASE)
#		MESSAGE(FATAL_ERROR "qjpeg plugin wasn't found!")
#	ENDIF()
#	SET(QJPEG_LIBRARIES "debug;${QJPEG_DEBUG};optimized;${QJPEG_RELEASE}")
#
	LIST(
		APPEND EXTRA_LIBS "${QT_QTMAIN_LIBRARY}"
		winmm imm32 ws2_32 ole32 oleaut32 uuid gdi32 comdlg32 winspool
	)
ENDIF()

# Note that ${JPEG_LIBRARY} must go after qjpeg plugin, because otherwise
# the GNU linker won't resolve symbols qjpeg needs from it.
LIST(APPEND EXTRA_LIBS ${TIFF_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${JPEG_LIBRARY})

IF(ENABLE_OPENGL)
	LIST(APPEND EXTRA_LIBS ${QT_QTOPENGL_LIBRARY} ${OPENGL_LIBRARIES})
ENDIF()

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

# crash_reporter is included unconditionally to collect translation sources from there.
ADD_SUBDIRECTORY(crash_reporter)
ADD_SUBDIRECTORY(dewarping)
ADD_SUBDIRECTORY(foundation)
ADD_SUBDIRECTORY(math)
ADD_SUBDIRECTORY(imageproc)
ADD_SUBDIRECTORY(interaction)
ADD_SUBDIRECTORY(zones)
ADD_SUBDIRECTORY(tests)

FILE(GLOB common_ui_files ui/ErrorWidget.ui)
FILE(GLOB gui_only_ui_files "ui/*.ui")
FOREACH(ui_file ${common_ui_files})
	LIST(REMOVE_ITEM gui_only_ui_files "${ui_file}")
ENDFOREACH()

SOURCE_GROUP("UI Files" FILES ${common_ui_files} ${gui_only_ui_files})
QT4_WRAP_UI(common_ui_sources ${common_ui_files})
QT4_WRAP_UI(gui_only_ui_sources ${gui_only_ui_files})
SET_SOURCE_FILES_PROPERTIES(${common_ui_sources} ${gui_only_ui_files} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

ADD_CUSTOM_TARGET(toplevel_ui_sources DEPENDS ${common_ui_sources} ${gui_only_ui_sources})

ADD_SUBDIRECTORY(filters/fix_orientation)
ADD_SUBDIRECTORY(filters/page_split)
ADD_SUBDIRECTORY(filters/deskew)
ADD_SUBDIRECTORY(filters/select_content)
ADD_SUBDIRECTORY(filters/page_layout)
ADD_SUBDIRECTORY(filters/output)

SET(resource_sources)
QT4_ADD_RESOURCES(resource_sources resources/resources.qrc)
SOURCE_GROUP("Generated" FILES ${common_ui_sources} ${gui_only_ui_sources} ${resource_sources})
SOURCE_GROUP("Resources" FILES resources/resources.qrc resources/icons/COPYING)
IF(WIN32)
	SOURCE_GROUP("Resources" FILES resources/win32/resources.rc)
ENDIF()

SET(
	common_sources
	BackgroundExecutor.cpp BackgroundExecutor.h
	OpenGLSupport.cpp OpenGLSupport.h
	PixmapRenderer.cpp PixmapRenderer.h
	BubbleAnimation.cpp BubbleAnimation.h
	ProcessingIndicationWidget.cpp ProcessingIndicationWidget.h
	NonOwningWidget.cpp NonOwningWidget.h
	Dpi.cpp Dpi.h Dpm.cpp Dpm.h
	SmartFilenameOrdering.cpp SmartFilenameOrdering.h
	AbstractRelinker.h
	RelinkablePath.cpp RelinkablePath.h
	ImageInfo.cpp ImageInfo.h
	ImageFileInfo.cpp ImageFileInfo.h
	ImageMetadata.cpp ImageMetadata.h
	RecentProjects.cpp RecentProjects.h
	OutOfMemoryHandler.cpp OutOfMemoryHandler.h
	CommandLine.cpp CommandLine.h
	PageSelectionAccessor.cpp PageSelectionAccessor.h
	PageSelectionProvider.h
	ContentSpanFinder.cpp ContentSpanFinder.h
	PhysicalTransformation.cpp PhysicalTransformation.h
	ImageTransformation.cpp ImageTransformation.h
	ImagePixmapUnion.h
	ImageViewBase.cpp ImageViewBase.h
	BasicImageView.cpp BasicImageView.h
	StageListView.cpp StageListView.h
	DebugImageView.cpp DebugImageView.h
	TabbedDebugImages.cpp TabbedDebugImages.h
	ThumbnailLoadResult.h
	ThumbnailPixmapCache.cpp ThumbnailPixmapCache.h
	ThumbnailBase.cpp ThumbnailBase.h
	ThumbnailFactory.cpp ThumbnailFactory.h
	IncompleteThumbnail.cpp IncompleteThumbnail.h
	ContentBoxPropagator.cpp ContentBoxPropagator.h
	PageOrientationPropagator.cpp PageOrientationPropagator.h
	DebugImages.cpp DebugImages.h
	ImageId.cpp ImageId.h
	PageId.cpp PageId.h
	PageInfo.cpp PageInfo.h
	BackgroundTask.cpp BackgroundTask.h
	ProcessingTaskQueue.cpp ProcessingTaskQueue.h
	PageSequence.cpp PageSequence.h
	StageSequence.cpp StageSequence.h
	ProjectPages.cpp ProjectPages.h
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
	ThreadPriority.cpp ThreadPriority.h
	FileNameDisambiguator.cpp FileNameDisambiguator.h
	OutputFileNameGenerator.cpp OutputFileNameGenerator.h
	PageRange.cpp PageRange.h
	SelectedPage.cpp SelectedPage.h
	Utils.cpp Utils.h
	PageView.h
	AutoManualMode.h
	AbstractCommand.h
	AbstractFilter.h
	BeforeOrAfter.h
	FilterResult.h
	CompositeCacheDrivenTask.h
	Margins.h
	ChangedStateItemDelegate.h
	PageOrderProvider.h
	PageOrderOption.h
	PayloadEvent.h
	filter_dc/AbstractFilterDataCollector.h
	filter_dc/ThumbnailCollector.h
	filter_dc/ContentBoxCollector.h
	filter_dc/PageOrientationCollector.h
	version.h
	config.h.in
	${common_ui_files}
)

SET(
	gui_only_sources
	Application.cpp Application.h
	SkinnedButton.cpp SkinnedButton.h
	RelinkablePathVisualization.cpp RelinkablePathVisualization.h
	RelinkingModel.cpp RelinkingModel.h
	RelinkingSortingModel.cpp RelinkingSortingModel.h
	RelinkingListView.cpp RelinkingListView.h
	RelinkingDialog.cpp RelinkingDialog.h
	SettingsDialog.cpp SettingsDialog.h
	ExportDialog.cpp ExportDialog.h
	FixDpiDialog.cpp FixDpiDialog.h
	LoadFilesStatusDialog.cpp LoadFilesStatusDialog.h
	ProjectCreationContext.cpp ProjectCreationContext.h
	ProjectOpeningContext.cpp ProjectOpeningContext.h
	OutOfMemoryDialog.cpp OutOfMemoryDialog.h
	ThumbnailSequence.cpp ThumbnailSequence.h
	ProjectFilesDialog.cpp ProjectFilesDialog.h
	NewOpenProjectPanel.cpp NewOpenProjectPanel.h
	SystemLoadWidget.cpp SystemLoadWidget.h
	MainWindow.cpp MainWindow.h
	main.cpp
)

SET(
	cli_only_sources
	ConsoleBatch.cpp ConsoleBatch.h
	main-cli.cpp
)

SOURCE_GROUP("Sources" FILES ${common_sources} ${gui_only_sources} ${cli_only_sources})
QT4_AUTOMOC(${common_sources} ${gui_only_sources} ${cli_only_sources})

SOURCE_GROUP("Special Headers" FILES version.h config.h.in)

IF(CMAKE_COMPILER_IS_GNUCXX)
	# There seems to be a bug in either gcc or boost that makes page sorting either crash
	# or go into infinite loop.  It's reproducible on a very simple test case on at least
	# gcc 4.4.3 and gcc 4.4.4.  It's not reproducible on gcc 4.5.1.
	# Adding -fno-strict-aliasing seems to fix it.
	SET_SOURCE_FILES_PROPERTIES(
		ThumbnailSequence.cpp PROPERTIES
		COMPILE_FLAGS "-fno-strict-aliasing"
	)
ENDIF()

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

ADD_LIBRARY(stcore STATIC ${common_sources} ${common_ui_sources})

ADD_EXECUTABLE(
	scantailor WIN32 ${gui_only_sources} ${common_ui_sources} ${gui_only_ui_sources}
	${resource_sources} ${win32_resource_file} resources/icons/COPYING
)

ADD_EXECUTABLE(scantailor-cli ${cli_only_sources} ${common_ui_sources})

# Note that order of static libraries matters with gcc.  Specifically,
# QJPEG_LIBRARIES needs to go before Qt libraries and EXTRA_LIBS needs
# to go after both of them.
TARGET_LINK_LIBRARIES(
	scantailor
	fix_orientation page_split deskew select_content page_layout output stcore
	dewarping zones interaction imageproc math foundation ${QJPEG_LIBRARIES}
	${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)
TARGET_LINK_LIBRARIES(
	scantailor-cli
	fix_orientation page_split deskew select_content page_layout output
	stcore dewarping zones interaction imageproc math foundation ${QJPEG_LIBRARIES}
	${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)
INSTALL(TARGETS scantailor scantailor-cli RUNTIME DESTINATION bin)

IF(ENABLE_CRASH_REPORTER)
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

	INCLUDE_DIRECTORIES(crash_reporter crash_reporter/google-breakpad)
	TARGET_LINK_LIBRARIES(scantailor crash_handler)
ENDIF(ENABLE_CRASH_REPORTER)

# Translations
TRANSLATION_SOURCES(
	scantailor
	${common_sources} ${gui_only_sources} ${cli_only_sources}
	${common_ui_files} ${gui_only_ui_files}
)
FILE(GLOB TRANSLATION_FILES translations/scantailor_*.ts)
FILE(GLOB CR_TRANSLATION_FILES translations/crashreporter_*.ts)

FINALIZE_TRANSLATION_SET(scantailor ${TRANSLATION_FILES})
FINALIZE_TRANSLATION_SET(crashreporter ${CR_TRANSLATION_FILES})
UPDATE_TRANSLATIONS_TARGET(update_translations scantailor crashreporter)

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
	"/\\\\.git/"
	"~$"
	"\\\\.pcs$"
	"TODO.txt"
	"CMakeLists.txt.user"
	"/doxygen/"
	"${CMAKE_BINARY_DIR}"
)
INCLUDE(CPack)

IF(WIN32)
	# Copy some DLLs to the staging dir.
	
	IF(MINGW)
		CHECK_CXX_ACCEPTS_FLAG("-m64" MINGW64)
		IF(NOT MINGW64)
			FIND_FILE(
				MINGWM10_DLL mingwm10.dll
				PATHS c:/MinGW/bin ENV PATH
				DOC "Full path to mingwm10.dll"
			)
			IF(NOT MINGWM10_DLL)
				MESSAGE(
					FATAL_ERROR
					"Could not find mingwm10.dll\n"
					"It shou 'bin' subdirectory of your MinGW installation."
				)
			ENDIF(NOT MINGWM10_DLL)
			COPY_TO_BUILD_DIR("${MINGWM10_DLL}")

			FIND_FILE(
				GCC_DW2_DLL libgcc_s_dw2-1.dll
				PATHS c:/MinGW/bin ENV PATH
				DOC "Full path to libgcc_s_dw2-1.dll"
			)
			IF(NOT GCC_DW2_DLL)
				SET(GCC_DW2_DLL "") # For NSIS
				MESSAGE(
					"Could not find libgcc_s_dw2-1.dll\n"
					"That's fine if your MinGW is old and just doesn't have it."
				)
			ELSE()
				COPY_TO_BUILD_DIR("${GCC_DW2_DLL}")
			ENDIF()
		ENDIF(NOT MINGW64)
	ENDIF(MINGW)
	
	SET(
		libs_copy_debug
		"${QT_QTCORE_LIBRARY_DEBUG}"
		"${QT_QTGUI_LIBRARY_DEBUG}"
		"${QT_QTXML_LIBRARY_DEBUG}"
		"${QT_QTNETWORK_LIBRARY_DEBUG}"
	)
	LIB_TO_DLL(libs_copy_debug ${libs_copy_debug})
	COPY_TO_BUILD_DIR(${libs_copy_debug} CONFIGURATIONS Debug)
	
	SET(
		libs_copy_nondebug
		"${QT_QTCORE_LIBRARY_RELEASE}"
		"${QT_QTGUI_LIBRARY_RELEASE}"
		"${QT_QTXML_LIBRARY_RELEASE}"
		"${QT_QTNETWORK_LIBRARY_RELEASE}"
	)
	LIB_TO_DLL(libs_copy_nondebug ${libs_copy_nondebug})
	COPY_TO_BUILD_DIR(${libs_copy_nondebug} CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
	
	# Qt's imageformat plugins. Note that png is built-in and for tiff we have our own code.
	COPY_TO_BUILD_DIR("${QT_QJPEG_PLUGIN_RELEASE}" SUBDIR imageformats CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
	COPY_TO_BUILD_DIR("${QT_QJPEG_PLUGIN_DEBUG}" SUBDIR imageformats CONFIGURATIONS Debug)
	
	IF(MINGW)
		SET(zlib_dll "${QT_BIN_DIR}/libz.dll")
	ELSE()
		SET(zlib_dll "${QT_BIN_DIR}/zdll.dll")
	ENDIF()
	SET(
		libs_copy_all_confs
		"${QT_BIN_DIR}/libpng.dll" "${QT_BIN_DIR}/libjpeg.dll"
		"${QT_BIN_DIR}/libtiff.dll" "${zlib_dll}"
	)
	COPY_TO_BUILD_DIR(${libs_copy_all_confs})
	
	# Generate the target that will actually do the copying.
	GENERATE_COPY_TO_BUILD_DIR_TARGET(copy_to_build_dir)
	
	# Installer for Windows.  Must go the last.
	ADD_SUBDIRECTORY(packaging/windows)
ENDIF(WIN32)
