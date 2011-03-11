PROJECT("Output Filter")

INCLUDE_DIRECTORIES(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

FILE(GLOB ui_files "ui/*.ui")
QT4_WRAP_UI(ui_sources ${ui_files})
SET_SOURCE_FILES_PROPERTIES(${ui_sources} PROPERTIES GENERATED TRUE)
SOURCE_GROUP("UI Files" FILES ${ui_files})
SOURCE_GROUP("Generated" FILES ${ui_sources})

SET(
	sources
	ApplyColorsDialog.cpp ApplyColorsDialog.h
	ChangeDpiDialog.cpp ChangeDpiDialog.h
	ImageView.cpp ImageView.h
	ImageViewTab.h
	TabbedImageView.cpp TabbedImageView.h
	Filter.cpp Filter.h
	OptionsWidget.cpp OptionsWidget.h
	Task.cpp Task.h
	CacheDrivenTask.cpp CacheDrivenTask.h
	OutputGenerator.cpp OutputGenerator.h
	OutputMargins.h
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
	PictureLayerProperty.cpp PictureLayerProperty.h
	PictureZonePropFactory.cpp PictureZonePropFactory.h
	PictureZonePropDialog.cpp PictureZonePropDialog.h
	PictureZoneComparator.cpp PictureZoneComparator.h
	PictureZoneEditor.cpp PictureZoneEditor.h
	FillColorProperty.cpp FillColorProperty.h
	FillZonePropFactory.cpp FillZonePropFactory.h
	FillZoneComparator.cpp FillZoneComparator.h
	FillZoneEditor.cpp FillZoneEditor.h
	ColorPickupInteraction.cpp ColorPickupInteraction.h
	DespeckleState.cpp DespeckleState.h
	DespeckleView.cpp DespeckleView.h
	DespeckleVisualization.cpp DespeckleVisualization.h
	DespeckleLevel.cpp DespeckleLevel.h
	DewarpingView.cpp DewarpingView.h
	DewarpingMode.cpp DewarpingMode.h
	ChangeDewarpingDialog.cpp ChangeDewarpingDialog.h
	DepthPerception.cpp DepthPerception.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(output STATIC ${sources} ${ui_sources})
ADD_DEPENDENCIES(output toplevel_ui_sources)

TRANSLATION_SOURCES(scantailor ${sources} ${ui_files})