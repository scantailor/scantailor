PROJECT("Foundation library")

SET(
	sources
	NonCopyable.h IntrusivePtr.h RefCountable.h
	ScopedIncDec.h ScopedDecInc.h
	Span.h VirtualFunction.h
	PerformanceTimer.cpp PerformanceTimer.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(foundation STATIC ${sources})
