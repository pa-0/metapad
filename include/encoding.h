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

#ifndef ENCODING_H
#define ENCODING_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef UNICODE
#include <wchar.h>
#endif

INT DecodeBase( BYTE base, LPCTSTR code, LPBYTE bin, INT len, BYTE extractMode, BYTE alignMode, BOOL showError, LPCTSTR* end );
INT EncodeBase( BYTE base, LPBYTE bin, LPTSTR code, INT len, LPBYTE* end );

void ReverseBytes(LPBYTE buffer, ULONG len);
WORD CheckBOM(LPBYTE pb, DWORD* len);
WORD GetBOM(LPBYTE* bom, WORD enc);
WORD GetNumKnownCPs();
WORD GetKnownCP(INT idx);
LPCTSTR GetCPName(WORD cp);
void PrintCPName(WORD cp, LPTSTR buf, LPCTSTR format);
BOOL IsTextUTF8(LPBYTE buf);

WORD GetLineFmt(LPCTSTR sz, DWORD len, WORD preferred, DWORD* nCR, DWORD* nLF, DWORD* nStrays, DWORD* nSub, BOOL* binary);
void ImportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays, DWORD nSub, BOOL* bufDirty);
void ExportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty);
#ifdef USE_RICH_EDIT
void ExportLineFmtLE(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty);
#else
#define ExportLineFmtLE	ExportLineFmt
#endif
LONG ExportLineFmtDelta(LPCTSTR sz, DWORD* chars, WORD lfmt);
void ImportBinary(LPTSTR sz, DWORD len);
void ExportBinary(LPTSTR sz, DWORD len);

DWORD DecodeText(LPBYTE* buf, DWORD bytes, DWORD* format, BOOL* bufDirty);
DWORD EncodeText(LPBYTE* buf, DWORD chars, DWORD format, BOOL* bufDirty, BOOL* truncated);

void ExpandDifMap(LPVOID map, WORD width, DWORD len);

void EvaHash(LPBYTE buf, DWORD len, LPBYTE hash);

LPTSTR FormatNumber(LONGLONG num, BOOL group, TCHAR sep, WORD buffer);

#endif
