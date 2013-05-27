PROJECT("Deskew Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	ImageView.cpp ImageView.h
	Thumbnail.cpp Thumbnail.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	Settings.cpp Settings.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	Dependencies.cpp Dependencies.h
	Params.cpp Params.h
	ApplyDialog.cpp ApplyDialog.h
)
SOURCE_GROUP("Sources" FILES ${sources})

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

ADD_LIBRARY(deskew STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(deskew Widgets Xml)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
