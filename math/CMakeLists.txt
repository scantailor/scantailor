PROJECT("Math library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	LinearSolver.cpp LinearSolver.h
	MatrixCalc.h
	HomographicTransform.h
	ToLineProjector.cpp ToLineProjector.h
	ReverseArcLengthMapper.cpp ReverseArcLengthMapper.h
	PolylineIntersector.cpp PolylineIntersector.h
	XSpline.cpp XSpline.h
	CylindricalSurfaceDewarper.cpp CylindricalSurfaceDewarper.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(math STATIC ${sources})
