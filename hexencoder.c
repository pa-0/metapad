/****************************************************************************/
/*                                                                          */
/*   metapad 3.6                                                            */
/*                                                                          */
/*   Copyright (C) 2013 Mario Rugiero					    */
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
 * @file hexencoder.c
 * @brief Binary to hexa converter.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef BUILD_METAPAD_UNICODE
#include <wchar.h>
#endif

/**
 * Converts an array bin of binary data, of size size bytes, into a string hex
 * of hexa characters of size [(2*size) + 1] ANSI or wide characters, depending
 * on definition of UNICODE macro. Null terminates the string.
 *
 * @param[in] bin The array of bytes to encode.
 * @param[in] size Size of the array to convert in bytes.
 * @param[out] hex Pointer to a string big enough to contain the encoded data.
 * @return Nothing.
 */
void BinToHex( const LPBYTE bin, DWORD size, TCHAR* hex )
{
	DWORD i;
	TCHAR HexIndex[] = _T("0123456789ABCDEF");
	for( i = 0; i < size; ++i ){
		hex[2*i] = HexIndex[(bin[i] >> 4) & 0x0F];
		hex[(2*i)+1] = HexIndex[bin[i] & 0x0F];
	}
	hex[(2*i)+2] = _T('\0');
}
