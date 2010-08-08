PROJECT(interaction)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	DraggableObject.h
	DraggablePoint.cpp DraggablePoint.h
	DraggableLineSegment.cpp DraggableLineSegment.h
	ObjectDragHandler.cpp ObjectDragHandler.h
	InteractionHandler.cpp InteractionHandler.h
	InteractionState.cpp InteractionState.h
	DragHandler.cpp DragHandler.h
	DragWatcher.cpp DragWatcher.h
	ZoomHandler.cpp ZoomHandler.h
	InteractiveXSpline.cpp InteractiveXSpline.h
)

SOURCE_GROUP(Sources FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(interaction STATIC ${sources})

TRANSLATION_SOURCES(scantailor ${sources})