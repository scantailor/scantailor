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
	LinearFunction.cpp LinearFunction.h
	QuadraticFunction.cpp QuadraticFunction.h
	XSpline.cpp XSpline.h
)
SOURCE_GROUP("Sources" FILES ${GENERIC_SOURCES})

SET(
	SPFIT_SOURCES
	spfit/references.txt
	spfit/FittableSpline.h
	spfit/FrenetFrame.cpp spfit/FrenetFrame.h
	spfit/ConstraintSet.cpp spfit/ConstraintSet.h
	spfit/SqDistApproximant.cpp spfit/SqDistApproximant.h
	spfit/ModelShape.h
	spfit/PolylineModelShape.cpp spfit/PolylineModelShape.h
	spfit/LinearForceBalancer.cpp spfit/LinearForceBalancer.h
	spfit/OptimizationResult.cpp spfit/OptimizationResult.h
	spfit/Optimizer.cpp spfit/Optimizer.h
	spfit/SplineFitter.cpp spfit/SplineFitter.h
)
SOURCE_GROUP("Sources\\Spline Fitting Framework" FILES ${SPFIT_SOURCES})

SET(
	ADIFF_SOURCES
	adiff/references.txt
	adiff/SparseMap.cpp adiff/SparseMap.h
	adiff/Function.cpp adiff/Function.h
)
SOURCE_GROUP("Sources\\Differentiation Framework" FILES ${ADIFF_SOURCES})

ADD_LIBRARY(math STATIC ${GENERIC_SOURCES} ${SPFIT_SOURCES} ${ADIFF_SOURCES})

ADD_SUBDIRECTORY(spfit/tests)
ADD_SUBDIRECTORY(adiff/tests)
