PROJECT("Output Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

FILE(GLOB ui_files "ui/*.ui")
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
ADD_SUBDIRECTORY(ui)

SET(
	sources
	ApplyColorsDialog.cpp ApplyColorsDialog.h
	ChangeDpiDialog.cpp ChangeDpiDialog.h
	ImageView.cpp ImageView.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	OutputGenerator.cpp OutputGenerator.h
	Settings.cpp Settings.h
	Thumbnail.cpp Thumbnail.h
	Utils.cpp Utils.h
	Params.cpp Params.h
	BlackWhiteOptions.cpp BlackWhiteOptions.h
	ColorGrayscaleOptions.cpp ColorGrayscaleOptions.h
	RenderParams.cpp RenderParams.h
	ColorParams.cpp ColorParams.h
	OutputImageParams.cpp OutputImageParams.h
	OutputFileParams.cpp OutputFileParams.h
	OutputParams.cpp OutputParams.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(output STATIC ${sources} ${ui_sources})

FILE(GLOB abs_sources ABSOLUTE ${sources})
FILE(GLOB abs_ui_files ABSOLUTE ${ui_files})
SET(
	TRANSLATION_SOURCES ${TRANSLATION_SOURCES}
	${abs_sources} ${abs_ui_files} PARENT_SCOPE
)
