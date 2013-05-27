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

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("Ui Files" FILES ${ui_files})

ADD_SUBDIRECTORY(ui)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}") # for ui files

IF(ENABLE_CRASH_REPORTER)
	QT5_WRAP_UI(ui_sources ${ui_files})

	ADD_EXECUTABLE(CrashReporter WIN32 ${crash_reporter_sources} ${ui_sources})
	QT5_USE_MODULES(CrashReporter Widgets Xml Network)
	TARGET_LINK_LIBRARIES(
		CrashReporter ${MAYBE_QTMAIN_LIBRARIES} ${PNG_LIBRARY} ${JPEG_LIBRARY} winmm imm32 ws2_32
	)
ENDIF()

TRANSLATION_SOURCES(crashreporter ${crash_reporter_sources} ${ui_files})
