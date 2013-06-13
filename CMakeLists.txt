project (metapad) 

# Required cmake version
cmake_minimum_required ( VERSION 2.6 )

if ( WIN32 )

if ( MINGW OR BORLAND OR WATCOM )
	set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -pg" )
	set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pg" )
	set ( FLAG "-" )
else ( MSVC )
	set ( FLAG "/" )
	set ( SOURCE ${SOURCE} ${CMAKE_SOURCE_DIR}/w32crt.c )
endif ()

set( LIBS ${LIBS} msvcrt comctl32 )

if ( DEBUG )
	add_definitions( ${FLAG}DDEBUG )
	message( "DEBUG defined." )
endif ()

if ( RICH_EDIT )
	add_definitions ( ${FLAG}DUSE_RICH_EDIT )
	message ( "Using Rich Edit Control." )
else ()
	message ( "Using Edit Proc Control." )
endif ()

if ( UNICODE )
	add_definitions( ${FLAG}DBUILD_METAPAD_UNICODE ${FLAG}DUNICODE ${FLAG}D_UNICODE )

	if ( MINGW )
		set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode" )
	endif ()

	message( "[EXPERIMENTAL] Unicode support enabled." )
else ()
	add_definitions( ${FLAG}DN_UNICODE ${FLAG}DNUNICODE )
endif ()

if ( STREAMING )
	add_definitions ( ${FLAG}DSTREAMING )
	message ( "[EXPERIMENTAL] Streaming support enabled." )
endif ()

if ( BOOKMARKS )
	add_definitions ( ${FLAG}DBOOKMARKS )
	message ( "[EXPERIMENTAL] Bookmarks support enabled." )
endif ()

set( EXE_NAME "metapad" )

include_directories( include )

set( SOURCE ${SOURCE}

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

target_link_libraries( ${EXE_NAME} ${LIBS} )

else ()
	message (FATAL_ERROR "Unsupported Platform")
endif ()
