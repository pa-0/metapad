project (metapad) 


# Required cmake version
cmake_minimum_required ( VERSION 2.6 )

if ( WIN32 )

set( EXE_NAME "metapad" )

include_directories( include )

set( SOURCE

	${CMAKE_SOURCE_DIR}/cdecode.c
	${CMAKE_SOURCE_DIR}/cencode.c
	${CMAKE_SOURCE_DIR}/external_viewers.c
	${CMAKE_SOURCE_DIR}/file_save.c
	${CMAKE_SOURCE_DIR}/language_plugin.c
	${CMAKE_SOURCE_DIR}/metapad.c
	${CMAKE_SOURCE_DIR}/settings_load.c
	${CMAKE_SOURCE_DIR}/settings_save.c

)

set ( RESOURCE ${CMAKE_SOURCE_DIR}/metapad.rc )

add_executable( ${EXE_NAME} ${SOURCE} ${HEADER} ${RESOURCE} )

target_link_libraries( ${EXE_NAME} comctl32 )

else ()
	message (FATAL_ERROR "Unsupported Platform")
endif ()
