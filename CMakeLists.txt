project (metapad) 

# Required cmake version
cmake_minimum_required ( VERSION 2.6 )

if ( WIN32 )

if ( MINGW || BORLAND || WATCOM )
	set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -pg -s" )
	set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pg" )
	set ( DEFINE "-D" )
else ( MSVC )
	set ( DEFINE "/D" )
endif ()


if ( UNICODE )
	add_definitions( ${DEFINE}USE_METAPAD_UNICODE )
	message( "[EXPERIMENTAL] Unicode support enabled." )
else ()
	add_definitions( ${DEFINE}N_UNICODE ${DEFINE}NUNICODE )
endif ()

if ( RICH_EDIT )
	add_definitions ( ${DEFINE}USE_RICH_EDIT )
	message ( "Using Rich Edit Control." )
else ()
	message ( "Using Edit Proc Control." )
endif ()

if ( STREAMING )
	add_definitions ( ${DEFINE}STREAMING )
	message ( "[EXPERIMENTAL] Streaming support enabled." )
endif ()

if ( BOOKMARKS )
	add_definitions ( ${DEFINE}BOOKMARKS )
	message ( "[EXPERIMENTAL] Bookmarks support enabled." )
endif ()

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

target_link_libraries( ${EXE_NAME} msvcrt comctl32 )

else ()
	message (FATAL_ERROR "Unsupported Platform")
endif ()
