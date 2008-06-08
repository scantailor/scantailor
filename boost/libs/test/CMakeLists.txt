ADD_LIBRARY (
	boost_test STATIC
	src/compiler_log_formatter.cpp src/debug.cpp
	src/exception_safety.cpp src/execution_monitor.cpp
	src/framework.cpp src/interaction_based.cpp src/logged_expectations.cpp
	src/plain_report_formatter.cpp src/progress_monitor.cpp
	src/results_collector.cpp src/results_reporter.cpp src/test_main.cpp
	src/test_tools.cpp src/unit_test_log.cpp src/unit_test_main.cpp
	src/unit_test_monitor.cpp src/unit_test_parameters.cpp
	src/unit_test_suite.cpp src/xml_log_formatter.cpp
	src/xml_report_formatter.cpp
)