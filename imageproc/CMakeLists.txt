PROJECT(imageproc)

SET(
	sources
	Constants.h Constants.cpp
	BinaryImage.cpp BinaryImage.h
	BinaryThreshold.cpp BinaryThreshold.h
	SlicedHistogram.cpp SlicedHistogram.h
	ByteOrder.h BWColor.h
	ConnComp.h Connectivity.h
	BitOps.cpp BitOps.h
	SeedFill.cpp SeedFill.h
	ConnCompEraser.cpp ConnCompEraser.h
	ConnCompEraserExt.cpp ConnCompEraserExt.h
	GrayImage.cpp GrayImage.h
	Grayscale.cpp Grayscale.h
	RasterOp.h GrayRasterOp.h RasterOpGeneric.h
	UpscaleIntegerTimes.cpp UpscaleIntegerTimes.h
	ReduceThreshold.cpp ReduceThreshold.h
	Shear.cpp Shear.h
	SkewFinder.cpp SkewFinder.h
	OrthogonalRotation.cpp OrthogonalRotation.h
	Scale.cpp Scale.h
	Transform.cpp Transform.h
	Morphology.cpp Morphology.h
	DentFinder.cpp DentFinder.h
	IntegralImage.h
	Binarize.cpp Binarize.h
	PolygonUtils.cpp PolygonUtils.h
	PolygonRasterizer.cpp PolygonRasterizer.h
	HoughLineDetector.cpp HoughLineDetector.h
	GaussBlur.cpp GaussBlur.h
	Sobel.h
	MorphGradientDetect.cpp MorphGradientDetect.h
	LeastSquaresFit.cpp LeastSquaresFit.h
	PolynomialLine.cpp PolynomialLine.h
	PolynomialSurface.cpp PolynomialSurface.h
	SavGolKernel.cpp SavGolKernel.h
	SavGolFilter.cpp SavGolFilter.h
	DrawOver.cpp DrawOver.h
	AdjustBrightness.cpp AdjustBrightness.h
	SEDM.cpp SEDM.h
	ConnectivityMap.cpp ConnectivityMap.h
	InfluenceMap.cpp InfluenceMap.h
	MaxWhitespaceFinder.cpp MaxWhitespaceFinder.h
	RastLineFinder.cpp RastLineFinder.h
	ColorInterpolation.cpp ColorInterpolation.h
	LocalMinMaxGeneric.h
	SeedFillGeneric.cpp SeedFillGeneric.h
	FindPeaksGeneric.h
	ColorMixer.h
	ColorForId.h
)

SOURCE_GROUP(Sources FILES ${sources})

ADD_LIBRARY(imageproc STATIC ${sources})

ADD_SUBDIRECTORY(tests)
