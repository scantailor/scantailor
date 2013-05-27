PROJECT("Select Content Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	ImageView.cpp ImageView.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	ApplyDialog.cpp ApplyDialog.h
	ContentBoxFinder.cpp ContentBoxFinder.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	Dependencies.cpp Dependencies.h
	Params.cpp Params.h
	Settings.cpp Settings.h
	Thumbnail.cpp Thumbnail.h
	PhysSizeCalc.cpp PhysSizeCalc.h
	OrderByWidthProvider.cpp OrderByWidthProvider.h
	OrderByHeightProvider.cpp OrderByHeightProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

ADD_LIBRARY(select_content STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(select_content Widgets Xml)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
