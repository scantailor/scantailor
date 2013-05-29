INCLUDE_DIRECTORIES(BEFORE ..)

SET(
	sources
	${CMAKE_SOURCE_DIR}/tests/main.cpp
	TestSqDistApproximant.cpp
)

SOURCE_GROUP("Sources" FILES ${sources})

SET(
	libs
	math ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${Boost_PRG_EXECUTION_MONITOR_LIBRARY} ${EXTRA_LIBS}
)

ADD_EXECUTABLE(spfit_tests ${sources})
QT5_USE_MODULES(spfit_tests Core)
TARGET_LINK_LIBRARIES(spfit_tests ${libs})
