PROJECT("Foundation library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	NonCopyable.h IntrusivePtr.h RefCountable.h
	AlignedArray.h
	FastQueue.h
	SafeDeletingQObjectPtr.h
	ScopedIncDec.h ScopedDecInc.h
	Span.h VirtualFunction.h FlagOps.h
	AutoRemovingFile.cpp AutoRemovingFile.h
	Proximity.cpp Proximity.h
	Property.h
	PropertyFactory.cpp PropertyFactory.h
	PropertySet.cpp PropertySet.h
	PerformanceTimer.cpp PerformanceTimer.h
	QtSignalForwarder.cpp QtSignalForwarder.h
	GridLineTraverser.cpp GridLineTraverser.h
	StaticPool.h
	DynamicPool.h
	NumericTraits.h
	VecNT.h
	VecT.h
	MatMNT.h
	MatT.h
	PriorityQueue.h
	Grid.h
	ValueConv.h
)
SOURCE_GROUP("Sources" FILES ${sources})
QT4_AUTOMOC(${sources})

ADD_LIBRARY(foundation STATIC ${sources})
