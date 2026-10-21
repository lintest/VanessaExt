add_executable(ProcessUtilsTest
	ProcessUtilsTest.cpp
	../src/ProcessUtils.cpp
	../src/ProcessUtils.h
)

target_include_directories(ProcessUtilsTest PRIVATE
	../src
	../include
)

target_compile_features(ProcessUtilsTest PRIVATE cxx_std_17)
target_link_libraries(ProcessUtilsTest PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

if(MSVC)
	target_compile_definitions(ProcessUtilsTest PRIVATE
		_WINDOWS
		UNICODE
		_UNICODE
		_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	)
	target_compile_options(ProcessUtilsTest PRIVATE /utf-8)
endif()

set_target_properties(ProcessUtilsTest PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
	PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

add_test(NAME ProcessUtilsTest COMMAND ProcessUtilsTest)
