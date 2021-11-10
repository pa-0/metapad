project (metapad) 

# Required cmake version
cmake_minimum_required ( VERSION 2.6 )

if ( WIN32 )

if ( MSVC )
else ( MINGW OR BORLAND OR WATCOM )
	set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -pg -s" )
	set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pg" )
endif ()

set ( LIBS ${LIBS} comctl32 )

if ( DEBUG )
	add_definitions( -DDEBUG -D_DEBUG )
	message( "DEBUG defined." )
else ()
	add_definitions( -DNDEBUG -D_NDEBUG )
endif ()

if ( RICH_EDIT )
	add_definitions ( -DUSE_RICH_EDIT )
	message ( "Using Rich Edit Control." )
else ()
	message ( "Using Edit Control." )
endif ()

if ( UNICODE )
	add_definitions( -DBUILD_METAPAD_UNICODE -DUNICODE -D_UNICODE )

	if ( MINGW )
		set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode" )
	endif ()

	message( "[EXPERIMENTAL] Unicode support enabled." )
else ()
	add_definitions( -DN_UNICODE -DNUNICODE )
endif ()

if ( STREAMING )
	add_definitions ( -DSTREAMING )
	message ( "[EXPERIMENTAL] Streaming support enabled." )
endif ()

if ( BOOKMARKS )
	add_definitions ( -DBOOKMARKS )
	message ( "[EXPERIMENTAL] Bookmarks support enabled." )
endif ()

if ( TABS )
	add_definitions ( -DUSE_TABS )
	message ( "[EXPERIMENTAL] Tabbed interface enabled." )
endif ()

set( EXE_NAME "metapad" )

include_directories( include )

set( SOURCE ${SOURCE}

	${CMAKE_SOURCE_DIR}/w32crt.c #No tiene utilidad en otro que no sea MSC, pero lo mantengo para asegurar que funcione. Una vez que deje de hacer cambios grandes, pasarlo solo a MSC (suma 1kb, en MSC puedo deshabilitar el runtime standard de C y usar eso en su lugar, salvando unos 40kb).
	${CMAKE_SOURCE_DIR}/encoding.c
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
