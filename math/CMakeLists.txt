PROJECT("Math library")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

SET(
	sources
	ToLineProjector.cpp ToLineProjector.h
)
SOURCE_GROUP("Sources" FILES ${sources})

ADD_LIBRARY(math STATIC ${sources})
