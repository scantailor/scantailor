ADD_LIBRARY (
	boost_signals STATIC
	src/connection.cpp src/named_slot_map.cpp src/signal_base.cpp
	src/slot.cpp src/trackable.cpp
)