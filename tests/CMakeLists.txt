INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	main.cpp TestContentSpanFinder.cpp
	TestSmartFilenameOrdering.cpp
	TestMatrixCalc.cpp
	../ContentSpanFinder.cpp ../ContentSpanFinder.h
	../SmartFilenameOrdering.cpp ../SmartFilenameOrdering.h
)

SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	imageproc math ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(tests ${sources})
QT5_USE_MODULES(tests Widgets)
TARGET_LINK_LIBRARIES(tests ${libs})
