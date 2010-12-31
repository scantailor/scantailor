PROJECT(CrashReporter)

ADD_DEFINITIONS(-D_UNICODE)
INCLUDE_DIRECTORIES(google-breakpad)

MACRO(LIST_ITEMS_PREPEND LIST PREFIX)
	SET(tmp_list_)
	FOREACH(item ${${LIST}})
		LIST(APPEND tmp_list_ "${PREFIX}${item}")
	ENDFOREACH(item)
	SET(${LIST} ${tmp_list_})
ENDMACRO(LIST_ITEMS_PREPEND)

SET(eh_sources exception_handler.cc exception_handler.h)
LIST_ITEMS_PREPEND(eh_sources google-breakpad/client/windows/handler/)

SET(cg_sources crash_generation_client.cc crash_generation_client.h)
LIST_ITEMS_PREPEND(cg_sources google-breakpad/client/windows/crash_generation/)

SET(common_sources guid_string.cc guid_string.h)
LIST_ITEMS_PREPEND(common_sources google-breakpad/common/windows/)

IF(ENABLE_CRASH_REPORTER)
	ADD_LIBRARY(crash_handler ${eh_sources} ${cg_sources} ${common_sources})
ENDIF(ENABLE_CRASH_REPORTER)

SET(
	crash_reporter_sources
	main.cpp CrashReportDialog.cpp CrashReportDialog.h
	MultipartFormData.cpp MultipartFormData.h
)
SOURCE_GROUP("Sources" FILES ${crash_reporter_sources})
QT4_AUTOMOC(${crash_reporter_sources})

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("Ui Files" FILES ${ui_files})
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

IF(ENABLE_CRASH_REPORTER)
	ADD_EXECUTABLE(CrashReporter WIN32 ${crash_reporter_sources} ${ui_sources})

	TARGET_LINK_LIBRARIES(
		CrashReporter ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTNETWORK_LIBRARY}
		${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTMAIN_LIBRARY} ${PNG_LIBRARY}
		${JPEG_LIBRARY} winmm imm32 ws2_32
	)
ENDIF(ENABLE_CRASH_REPORTER)

TRANSLATION_SOURCES(crashreporter ${crash_reporter_sources} ${ui_files})