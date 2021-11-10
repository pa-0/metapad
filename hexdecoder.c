/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
/*                                                                          */
/*   Copyright (C) 2021 SoBiT Corp                                          */
/*   Copyright (C) 2013 Mario Rugiero                                       */
/*   Copyright (C) 1999-2011 Alexander Davidson                             */
/*                                                                          */
/*   This program is free software: you can redistribute it and/or modify   */
/*   it under the terms of the GNU General Public License as published by   */
/*   the Free Software Foundation, either version 3 of the License, or      */
/*   (at your option) any later version.                                    */
/*                                                                          */
/*   This program is distributed in the hope that it will be useful,        */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/*   GNU General Public License for more details.                           */
/*                                                                          */
/*   You should have received a copy of the GNU General Public License      */
/*   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
/*                                                                          */
/****************************************************************************/

/**
 * @file hexdecoder.c
 * @brief Hexa to binary converter.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef BUILD_METAPAD_UNICODE
#include <wchar.h>
#endif

#include "include/strings.h"
#include "include/macros.h"

extern HWND hwnd;

/**
 * Decodes a single character (representing only 4 bits) to binary.
 *
 * @param[in] hexchar The character to decode.
 * @return The numeric value hexchar represents. If this is 255, it means
 * hexchar is an invalid hexadecimal character.
 * @note Case insensitive.
 */
static BYTE HexBaseToBin( TCHAR hexchar )
{
	switch( hexchar ){
		case _T('0'):
			return 0;
		case _T('1'):
			return 1;
		case _T('2'):
			return 2;
		case _T('3'):
			return 3;
		case _T('4'):
			return 4;
		case _T('5'):
			return 5;
		case _T('6'):
			return 6;
		case _T('7'):
			return 7;
		case _T('8'):
			return 8;
		case _T('9'):
			return 9;
		case _T('A'):
		case _T('a'):
			return 10;
		case _T('B'):
		case _T('b'):
			return 11;
		case _T('C'):
		case _T('c'):
			return 12;
		case _T('D'):
		case _T('d'):
			return 13;
		case _T('E'):
		case _T('e'):
			return 14;
		case _T('F'):
		case _T('f'):
			return 15;
		default:
			return 255;
	}
}

/**
 * Decodes a string of ANSI or wide characters, depending on definition of the
 * UNICODE macro, representing an hexadecimal array of binary data, to the array
 * it represents.
 *
 * @param[in] hex Null terminated string to decode.
 * @param[out] bin Pointer to the array of bytes where the data will be stored.
 * @return Size of decoded array.
 */
DWORD HexToBinEx( LPCTSTR hex, LPBYTE bin, BOOL ignoreParity )
{
	DWORD i, size;
	BYTE j;
	size = lstrlen( hex );
	if( size == 0 ) return 0;
	else if( !ignoreParity && size % 2 ){
		ERROROUT(_T("Invalid hex string!"));
		return 0;
	} else size = size/2;
	for( i = 0; i < size; ++i ){
		if( ( j = HexBaseToBin( hex[2*i] ) ) == 255 ){
			ERROROUT( _T("Invalid hex character!") );
			return i;
		} else
			bin[i] = j << 4;
		if( ( j = HexBaseToBin( hex[(2*i) + 1] ) ) == 255 ){
			ERROROUT( _T("Invalid hex character!") );
			return i;
		} else
			bin[i] |= j;
	}
	return i;
}

DWORD HexToBin( LPCTSTR hex, LPBYTE bin ){
	return HexToBinEx(hex, bin, FALSE);
}