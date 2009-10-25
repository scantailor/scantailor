PROJECT("Foundation library")

SET(
	sources
	NonCopyable.h IntrusivePtr.h RefCountable.h
	AlignedArray.h
	SafeDeletingQObjectPtr.h
	ScopedIncDec.h ScopedDecInc.h
	Span.h VirtualFunction.h FlagOps.h
	Proximity.cpp Proximity.h
	Property.h
	PropertyFactory.cpp PropertyFactory.h
	PropertySet.cpp PropertySet.h
	PerformanceTimer.cpp PerformanceTimer.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(foundation STATIC ${sources})
