INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	main.cpp TestContentSpanFinder.cpp
	TestSmartFilenameOrdering.cpp
	../ContentSpanFinder.cpp ../ContentSpanFinder.h
	../SmartFilenameOrdering.cpp ../SmartFilenameOrdering.h
)

SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	imageproc boost_test ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY}
	${EXTRA_LIBS}
)

ADD_EXECUTABLE(tests ${sources})
TARGET_LINK_LIBRARIES(tests ${libs})
