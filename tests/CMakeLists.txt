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
	${Boost_PRG_EXECUTION_MONITOR_LIBRARY}
	${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(tests ${sources})
TARGET_LINK_LIBRARIES(tests ${libs})
