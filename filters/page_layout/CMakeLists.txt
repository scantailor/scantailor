PROJECT("Page Layout Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

FILE(GLOB ui_files "ui/*.ui")
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
SOURCE_GROUP("UI Files" FILES ${ui_files})
SOURCE_GROUP("Generated" FILES ${ui_sources})

SET(
	sources
	Utils.cpp Utils.h
	ImageView.cpp ImageView.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	Params.cpp Params.h
	Settings.cpp Settings.h
	Thumbnail.cpp Thumbnail.h
	ApplyDialog.cpp ApplyDialog.h
	Alignment.cpp Alignment.h
	OrderByWidthProvider.cpp OrderByWidthProvider.h
	OrderByHeightProvider.cpp OrderByHeightProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(page_layout STATIC ${sources} ${ui_sources})

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})