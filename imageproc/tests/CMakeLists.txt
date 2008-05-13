INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	main.cpp
	TestBinaryImage.cpp TestReduceThreshold.cpp
	TestSlicedHistogram.cpp
	TestConnCompEraser.cpp TestConnCompEraserExt.cpp
	TestGrayscale.cpp
	TestRasterOp.cpp TestShear.cpp
	TestOrthogonalRotation.cpp
	TestSkewFinder.cpp
	TestScale.cpp
	TestTransform.cpp
	TestMorphology.cpp
	TestDentFinder.cpp
	Utils.cpp Utils.h
)
SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	imageproc boost_test ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY}
	${EXTRA_LIBS}
)

ADD_EXECUTABLE(tests ${sources})
TARGET_LINK_LIBRARIES(tests ${libs})
