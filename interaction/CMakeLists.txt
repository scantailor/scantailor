PROJECT(interaction)

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

ADD_LIBRARY(interaction STATIC ${sources})
