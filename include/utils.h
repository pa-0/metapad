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

#ifndef UTILS_H
#define UTILS_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>
#ifdef UNICODE
#include <wchar.h>
#endif
#ifdef _DEBUG
#include <stdio.h>
#endif
#include "globals.h"
#include "macros.h"
#include "resource.h"

LPCTSTR kemptyStr;

LPVOID kallocex(DWORD len, BOOL zeroMem);
BOOL kfree(LPVOID* mem);
LPTSTR kstrdupex(LPTSTR* tgt, LPCTSTR src, INT add, INT tgtOfs, BOOL emptyAsNull);
BOOL kgrowbuf(LPVOID* buf, DWORD width, DWORD len, DWORD* alen, LPVOID* bpos, DWORD pad);
INT kbinsearch(LPVOID list[], LPVOID tgt, DWORD width, DWORD l, DWORD r, BOOL insert);
VOID kdebuglog(INT id, LPVOID* data);

#define kalloc(len) kallocex(len, FALSE)
#define kallocz(len) kallocex(len, TRUE)
#define kallocs(chars) (LPTSTR)kallocex((chars) * sizeof(TCHAR), FALSE)
#define kallocsz(chars) (LPTSTR)kallocex((chars) * sizeof(TCHAR), TRUE)
#define kcalloc(len, width) kallocex(len * width, FALSE)
#define kcallocz(len, width) kallocex(len * width, TRUE)
#define kstrdup(tgt, src) kstrdupex(tgt, src, 1, 0, FALSE)
#define kstrdupa(tgt, src, add) kstrdupex(tgt, src, add, 0, FALSE)
#define kstrdupo(tgt, src, tgtOfs) kstrdupex(tgt, src, 1, tgtOfs, FALSE)
#define kstrdupao(tgt, src, add, tgtOfs) kstrdupex(tgt, src, add, tgtOfs, FALSE)
#define kstrdupnul(tgt, src) kstrdupex(tgt, src, 1, 0, TRUE)


LPCTSTR GetString(WORD uID);

#endif

