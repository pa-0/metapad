/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
/*                                                                          */
/*   Copyright (C) 2021-2024 SoBiT Corp                                     */
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

#ifndef MACROS_H
#define MACROS_H

#define CRITOUT(x) MessageBox(NULL, x, GetString(STR_METAPAD), MB_OK | MB_ICONSTOP)
#define ERROROUT(x) MessageBox(hwnd, x, GetString(STR_METAPAD), MB_OK | MB_ICONEXCLAMATION)
#define MSGOUT(x) MessageBox(hwnd, x, GetString(STR_METAPAD), MB_OK | MB_ICONINFORMATION)
#define DBGOUT(x, y) MessageBox(hwnd, x, y, MB_OK | MB_ICONEXCLAMATION)

#define MAX(x,y)		((x)>(y)?(x):(y))
#define MIN(x,y)		((x)<(y)?(x):(y))
#define WIDTH(x)		(x.right - x.left + 1)
#define HEIGHT(x)		(x.bottom - x.top + 1)
#define ARRLEN(x)		(sizeof(x)/sizeof(*(x)))

#define SCNUL(x)		SCNULD(x, kemptyStr)
#define SCNUL8(x)		SCNULD(x, _T("        "))
#define SCNULD(x, def)	(x ? x : def)

#define RAND() ((randVal = randVal * 214013L + 2531011L) >> 16)


#ifdef _DEBUG

#define DUMP(o, l, f) {\
	DWORD d;\
	HANDLE hFile = (HANDLE)CreateFile((f)?(f):(_T("__DUMP")), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);\
	WriteFile(hFile, o, l, &d, NULL);\
	SetEndOfFile(hFile);\
	CloseHandle(hFile);\
	}

#endif

#endif
