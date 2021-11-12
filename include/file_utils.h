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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#ifdef USE_RICH_EDIT
#include <richedit.h>
#endif
#include "include/metapad.h"

void MakeNewFile(void);

DWORD CalcTextSize(LPCTSTR* szText, DWORD estBytes, WORD encoding, BOOL unix, BOOL inclBOM, DWORD* numChars);
DWORD GetTextChars(LPCTSTR szText, BOOL unix);
void ExpandFilename(LPCTSTR szBuffer, LPTSTR* szOut);
BOOL SearchFile(LPCTSTR szText, BOOL bCase, BOOL bDown, BOOL bWholeWord, LPBYTE pbFindSpec);
DWORD ReplaceAll(HWND owner, DWORD nOps, DWORD recur, LPCTSTR* szFind, LPCTSTR* szRepl, LPBYTE* pbFindSpec, LPBYTE* pbReplSpec, LPTSTR szMsgBuf, BOOL selection, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen, LPCTSTR header, LPCTSTR footer);
DWORD StrReplace(LPCTSTR szIn, LPTSTR* szOut, DWORD* bufLen, LPCTSTR szFind, LPCTSTR szRepl, LPBYTE pbFindSpec, LPBYTE pbReplSpec, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen);
void SetFileFormat(int nFormat);

LPCTSTR GetShadowBuffer(DWORD* len);
LPCTSTR GetShadowRange(LONG min, LONG max, LONG line, DWORD* len);
LPCTSTR GetShadowSelection(DWORD* len, CHARRANGE* pcr);
LPCTSTR GetShadowLine(LONG line, LONG cp, DWORD* len, LONG* lineout, CHARRANGE* pcr);

DWORD GetColNum(LONG cp, LONG line, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr);
DWORD GetCharIndex(DWORD col, LONG line, LONG cp, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr);

#endif
