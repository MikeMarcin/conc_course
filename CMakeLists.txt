cmake_minimum_required( VERSION 3.8 )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-utils/Modules )
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project( conc_course )

find_package( GBenchmark REQUIRED )
set( BENCHMARK_INCLUDES 
		${GBenchmark_INCLUDE_DIRS} )

if(WIN32)
	set( SHLWAPI Shlwapi.lib ) 
else()
	set( SHLWAPI )
endif()

set( BENCHMARK_LIBRARIES
		${SHLWAPI}
		${GBenchmark_LIBRARIES} )

file( GLOB lectures lecture_* )
foreach( name ${lectures} )
	file( GLOB_RECURSE sources ${name}/*.[hic]pp )

	get_filename_component( name ${name} NAME )
	set( lecture_project_name ${name} )
	
	include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/source ${BENCHMARK_INCLUDES}  )
#	add_executable( ${lecture_project_name} WIN32 ${sources} )
	add_executable( ${lecture_project_name} ${sources} )
	target_link_libraries( ${lecture_project_name} ${project_name} ${BENCHMARK_LIBRARIES} )
	set_target_properties( ${lecture_project_name} PROPERTIES FOLDER lectures )

#	if(MSVC)
#		set_target_properties( ${lecture_project_name} PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
#	endif(MSVC)
endforeach()
