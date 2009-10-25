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
	ZoomHandler.cpp ZoomHandler.h
)

SOURCE_GROUP(Sources FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(interaction STATIC ${sources})

FILE(GLOB abs_sources ABSOLUTE ${sources})
SET(
	TRANSLATION_SOURCES ${TRANSLATION_SOURCES}
	${abs_sources} PARENT_SCOPE
)
