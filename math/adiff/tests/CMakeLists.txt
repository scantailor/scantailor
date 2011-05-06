INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	${CMAKE_SOURCE_DIR}/tests/main.cpp
	TestHessians.cpp
)

SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	math ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${Boost_PRG_EXECUTION_MONITOR_LIBRARY}
	${QT_QTCORE_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(adiff_tests ${sources})
TARGET_LINK_LIBRARIES(adiff_tests ${libs})
