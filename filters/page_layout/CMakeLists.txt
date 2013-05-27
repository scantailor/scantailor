PROJECT("Page Layout Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

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

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

ADD_LIBRARY(page_layout STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(page_layout Widgets Xml)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
