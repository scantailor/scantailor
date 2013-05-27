PROJECT("Page Split Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	SplitLineObject.h
	ImageView.cpp ImageView.h
	Thumbnail.cpp Thumbnail.h
	Params.cpp Params.h
	Dependencies.cpp Dependencies.h
	PageLayout.cpp PageLayout.h
	PageLayoutEstimator.cpp PageLayoutEstimator.h
	VertLineFinder.cpp VertLineFinder.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	SplitModeDialog.cpp SplitModeDialog.h
	Settings.cpp Settings.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	LayoutType.cpp LayoutType.h
	UnremoveButton.cpp UnremoveButton.h
	OrderBySplitTypeProvider.cpp OrderBySplitTypeProvider.h
)
SOURCE_GROUP("Sources" FILES ${sources})

FILE(GLOB ui_files "ui/*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})

ADD_LIBRARY(page_split STATIC ${sources} ${ui_sources})
QT5_USE_MODULES(page_split Widgets Xml)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})
