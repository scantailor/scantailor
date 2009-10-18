PROJECT(zones)

SET(
	sources
	SplineVertex.cpp SplineVertex.h
	SplineSegment.cpp SplineSegment.h
	Spline.cpp Spline.h
	BasicSplineVisualizer.cpp BasicSplineVisualizer.h
)

SOURCE_GROUP(Sources FILES ${sources})

ADD_LIBRARY(zones STATIC ${sources})
