PROJECT("Math library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	GENERIC_SOURCES
	LinearSolver.cpp LinearSolver.h
	MatrixCalc.h
	HomographicTransform.h
	SidesOfLine.cpp SidesOfLine.h
	ToLineProjector.cpp ToLineProjector.h
	ArcLengthMapper.cpp ArcLengthMapper.h
	LineIntersectionScalar.cpp LineIntersectionScalar.h
	LineBoundedByRect.cpp LineBoundedByRect.h
	PolylineIntersector.cpp PolylineIntersector.h
	XSpline.cpp XSpline.h
)
SOURCE_GROUP("Sources" FILES ${GENERIC_SOURCES})

SET(
	SPFIT_SOURCES
	spfit/references.txt
	spfit/FittableSpline.h
	spfit/SqDistApproximant.cpp spfit/SqDistApproximant.h
	spfit/ModelShape.h
	spfit/PolylineModelShape.cpp spfit/PolylineModelShape.h
	spfit/Optimizer.cpp spfit/Optimizer.h
	spfit/SplineFitter.cpp spfit/SplineFitter.h
)
SOURCE_GROUP("Sources\\Spline Fitting Framework" FILES ${SPFIT_SOURCES})

ADD_LIBRARY(math STATIC ${GENERIC_SOURCES} ${SPFIT_SOURCES})

ADD_SUBDIRECTORY(spfit/tests)
