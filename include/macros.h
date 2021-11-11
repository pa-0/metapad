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

#ifndef MACROS_H
#define MACROS_H

#define ERROROUT(_x) MessageBox(hwnd, _x, STR_METAPAD, MB_OK | MB_ICONEXCLAMATION)
#define MSGOUT(_x) MessageBox(hwnd, _x, STR_METAPAD, MB_OK | MB_ICONINFORMATION)
#define DBGOUT(_x, _y) MessageBox(hwnd, _x, _y, MB_OK | MB_ICONEXCLAMATION)

#define MAX(x,y)		((x)>(y)?(x):(y))
#define MIN(x,y)		((x)<(y)?(x):(y))
#define WIDTH(x)		(x.right - x.left + 1)
#define HEIGHT(x)		(x.bottom - x.top + 1)

#define FREE(x) {\
	/*if (x) printf("FREE %08X : %.32ls\n", x, x);*/\
	if (x) HeapFree(globalHeap, 0, (HGLOBAL)x);\
	x = NULL; }
#define SSTRCPY(tgt, src) SSTRCPYA(tgt, src, 1)
#define SSTRCPYA(tgt, src, add) {\
	if (tgt) HeapFree(globalHeap, 0, (HGLOBAL)tgt);\
	if (src && src[0]) {\
		tgt = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(src)+1) * sizeof(TCHAR));\
		lstrcpy(tgt, src);\
	} else tgt = NULL; }
#define SCNUL(x)		SCNULD(x, _T(""))
#define SCNULD(x, def)	(x ? x : def)
/*#define ALLOCF(x, t, f, sz) {\
	x = (t)HeapAlloc(globalHeap, f, sz);\
	if (!x) { ReportLastError(); x = NULL; }\
}
#define ALLOCF(t, f, sz) ((t)HeapAlloc(globalHeap, f, sz))
#define ALLOC(t, sz) ALLOCF(t, 0, sz)
#define ALLOCZ(t, sz) ALLOCF(t, HEAP_ZERO_MEMORY, sz)
#define ALLOCS(sz) ALLOC(LPTSTR, ((sz)+1) * sizeof(TCHAR))
#define ALLOCSZ(sz) ALLOCZ(LPTSTR, ((sz)+1) * sizeof(TCHAR))*/

#define RAND() ((randVal = randVal * 214013L + 2531011L) >> 16)

#endif
