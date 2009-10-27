IF(MSVC)
	# If we want reliable stack traces, we need /Oy-
	# We do it only for RelWithDebInfo, because configuration is used with
	# the built-in crash reporter.
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MD /Zi /O2 /Ob1 /D NDEBUG /Oy-")
ENDIF()
