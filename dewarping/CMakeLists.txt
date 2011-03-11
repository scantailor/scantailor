PROJECT("Dewarping library")

SET(
	sources
	Curve.cpp Curve.h
	DistortionModel.cpp DistortionModel.h
	DistortionModelBuilder.cpp DistortionModelBuilder.h
	DetectVertContentBounds.cpp DetectVertContentBounds.h
	TowardsLineTracer.cpp TowardsLineTracer.h
	TextLineTracer.cpp TextLineTracer.h
	TextLineRefiner.cpp TextLineRefiner.h
	TopBottomEdgeTracer.cpp TopBottomEdgeTracer.h
	CylindricalSurfaceDewarper.cpp CylindricalSurfaceDewarper.h
	DewarpingPointMapper.cpp DewarpingPointMapper.h
	RasterDewarper.cpp RasterDewarper.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(dewarping STATIC ${sources})
