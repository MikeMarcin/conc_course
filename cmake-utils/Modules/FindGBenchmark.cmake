include( FindPackageHandleStandardArgs )

set(_gbench_ROOT
    "C:/code/google/benchmark"
  )

if(CMAKE_CL_64)
	set(_gbench_ARCH "x64")
else()
	set(_gbench_ARCH "x86")
endif()

if (MSVC_VERSION EQUAL 1910 OR MSVC_VERSION EQUAL 1911)
	set(_gbench_COMPILER "vc141")
elseif (MSVC14)
	set(_gbench_COMPILER "vc14")
elseif (MSVC12)
	set(_gbench_COMPILER "vc12")
elseif (MSVC11)
	set(_gbench_COMPILER "vc11")
elseif (MSVC10)
	set(_gbench_COMPILER "vc10")
elseif (MSVC90)
	set(_gbench_COMPILER "vc9")
else()
	message(SEND_ERROR "Unknown compiler for Google Benchmark")
endif()

find_path( GBenchmark_INCLUDE_DIRS benchmark/benchmark.h PATHS ${_gbench_ROOT}/include )

string(REGEX REPLACE "include" "lib" GBenchmark_LIBRARY_DIRS "${GBenchmark_INCLUDE_DIRS}")

set( GBenchmark_LIBRARY_DIRS ${GBenchmark_LIBRARY_DIRS}/${_gbench_COMPILER}/${_gbench_ARCH} )

unset( GBenchmark_LIBRARIES CACHE )

function( find_gbench_library lib_name )
	find_library( GBenchmark_LIBRARIES_DEBUG_${lib_name} ${lib_name} PATHS ${GBenchmark_LIBRARY_DIRS} PATH_SUFFIXES "debug" )
	find_library( GBenchmark_LIBRARIES_RELEASE_${lib_name} ${lib_name} PATHS ${GBenchmark_LIBRARY_DIRS} PATH_SUFFIXES "release" )

	set( GBenchmark_LIBRARIES ${GBenchmark_LIBRARIES} optimized "${GBenchmark_LIBRARIES_RELEASE_${lib_name}}" debug "${GBenchmark_LIBRARIES_DEBUG_${lib_name}}" CACHE STRING "GBenchmark libs" FORCE )
endfunction()

find_gbench_library( benchmark )
# find_gbench_library( output_test_helper )

find_package_handle_standard_args( GBenchmark DEFAULT_MSG GBenchmark_INCLUDE_DIRS GBenchmark_LIBRARIES )