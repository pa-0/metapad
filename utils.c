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

#include "include/utils.h"

LPCTSTR kemptyStr = _T("");

#define KINITIALALLOCS 64

LPVOID* _kalloct = NULL;
DWORD _kallocn = 0, _kalloca = KINITIALALLOCS;
#ifdef _DEBUG
DWORD* _kallocts = NULL;
DWORD _kallocsz = 0;
#endif

LPVOID kallocex(DWORD len, BOOL zeroMem) {
#ifdef _DEBUG
	DWORD _n = _kalloca;
#endif
	INT i, p;
	LPVOID r = HeapAlloc(globalHeap, zeroMem ? HEAP_ZERO_MEMORY : 0, len);
	if (!r) CRITOUT(GetString(IDS_UTILS_NOMEM));
	else {
		if (!_kalloct) {
			_kalloct = (LPVOID*)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, KINITIALALLOCS*sizeof(LPVOID));
#ifdef _DEBUG
			_kallocts = (DWORD*)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, KINITIALALLOCS*sizeof(DWORD));
#endif
		}
		if (++_kallocn > _kalloca) {
			kgrowbuf(&(LPVOID)_kalloct, sizeof(LPVOID), _kallocn, &_kalloca, NULL, 0);
#ifdef _DEBUG
			kgrowbuf(&_kallocts, sizeof(DWORD), _kallocn, &_n, NULL, 0);
#endif
		}
		p = kbinsearch(_kalloct, r, 0, 0, _kallocn-1, TRUE);
		for (i=_kallocn-1; i>p; i--) {
			_kalloct[i] = _kalloct[i-1];
#ifdef _DEBUG
			_kallocts[i] = _kallocts[i-1];
#endif
		}
		_kalloct[p] = r;
#ifdef _DEBUG
		_kallocts[p] = len;
		_kallocsz += len;
#endif
	}
	return r;
}

BOOL kfree(LPVOID* mem) {
	INT p;
	BOOL r = FALSE;
	if (mem && *mem && *mem != kemptyStr) {
		if (0>(p = kbinsearch(_kalloct, *mem, 0, 0, _kallocn, FALSE))) {
			if (options.bDebug) kdebuglog(1, mem);
#ifdef _DEBUG
			printf("BAD FREE1 %08X : %08X : \"%.32ls\"\n", mem, *mem, *mem);
#endif
			return r;
		}
		memcpy(&_kalloct[p], &_kalloct[p+1], (--_kallocn-p)*sizeof(LPVOID));
#ifdef _DEBUG
		_kallocsz -= _kallocts[p];
		memcpy(&_kallocts[p], &_kallocts[p+1], (_kallocn-p)*sizeof(DWORD));
#endif
		r = HeapFree(globalHeap, 0, (HGLOBAL)*mem);
		if (!r && options.bDebug) kdebuglog(2, mem);
#ifdef _DEBUG
		if (!r) printf("BAD FREE2 %08X : %08X : \"%.32ls\"\n", mem, *mem, *mem);
#endif
		*mem = NULL;
	}
	return r;
}

LPTSTR kstrdupex(LPTSTR* tgt, LPCTSTR src, INT add, INT tgtOfs, BOOL emptyAsNull) {
	INT chars;
	if (*tgt) kfree(tgt);
	if (!src || (!*src && emptyAsNull) || tgtOfs < 0 || (chars = lstrlen(src)+add+tgtOfs) < 1) return NULL;
	if (*src) {
		*tgt = kallocs(chars);
		if (*tgt) lstrcpy(*tgt+tgtOfs, src);
	} else *tgt = (LPTSTR)kemptyStr;
	return *tgt;
}

BOOL kgrowbuf(LPVOID* buf, DWORD width, DWORD len, DWORD* alen, LPVOID* bpos, DWORD pad){
	LPVOID new;
	DWORD p, intern=0;
	if (!alen || len <= *alen || !buf || !*buf || width < 1) return FALSE;
	if (*buf == _kalloct) intern=1;
#ifdef _DEBUG
	if (*buf == _kallocts) intern=1;
#endif
	if (bpos) p = ((LPBYTE)*bpos - (LPBYTE)*buf);
	else p = MIN(len,*alen)*width;
	*alen = ((len+1) / 2) * 3;
	if (intern) { if (!(new = HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (*alen+pad) * width))) return FALSE; }
	else { if (!(new = kalloc((*alen+pad) * width))) return FALSE; }
	memcpy(new, *buf, p + pad*width);
	if (bpos) *bpos = &((LPBYTE)new)[p];
	if (intern) HeapFree(globalHeap, 0, (HGLOBAL)*buf);
	else kfree(buf);
	*buf = new;
	return TRUE;
}

INT kbinsearch(LPVOID list[], LPVOID tgt, DWORD width, DWORD l, DWORD r, BOOL insert){
	DWORD m, ol=r;
	if (!list) return -1;
	switch (width) {
	case 4:
		while (l < r) {
			m = l + (r-l)/2;
			if (((DWORD*)list)[m] < (DWORD)tgt) l = m+1;
			else r = m;
		}
		if (insert || (l < ol && ((DWORD*)list)[l] == (DWORD)tgt)) return l;
		break;
	case 2:
		while (l < r) {
			m = l + (r-l)/2;
			if (((WORD*)list)[m] < (WORD)tgt) l = m+1;
			else r = m;
		}
		if (insert || (l < ol && ((WORD*)list)[l] == (WORD)tgt)) return l;
		break;
	case 1:
		while (l < r) {
			m = l + (r-l)/2;
			if (((LPBYTE)list)[m] < (BYTE)tgt) l = m+1;
			else r = m;
		}
		if (insert || (l < ol && ((LPBYTE)list)[l] == (BYTE)tgt)) return l;
		break;
	case 0:
		while (l < r) {
			m = l + (r-l)/2;
			if (list[m] < tgt) l = m+1;
			else r = m;
		}
		if (insert || (l < ol && list[l] == tgt)) return l;
		break;
	}
	return -1;
}

VOID kdebuglog(INT id, LPVOID* data) {
	HANDLE f;
	DWORD ch=0;
	if (!options.bDebug) return;
	*gTmpBuf = 0;
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, GetString(IDSD_CUSTOMDATE), gTmpBuf, 100);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, gTmpBuf, gTmpBuf, 100);
	lstrcat(gTmpBuf, _T("\t"));
	switch(id) {
		case 1:
		case 2:
			wsprintf(gTmpBuf+100, GetString(STR_DEBUG_BADFREE), id, data, *data, *data);
			ch = lstrlen(gTmpBuf+100);
			lstrcpyn(gTmpBuf+100+ch, *data, 48);
			gTmpBuf[148+ch] = 0;
			break;
	}
	lstrcat(gTmpBuf, gTmpBuf+100);
	lstrcat(gTmpBuf, _T("\n"));
//#ifdef UNICODE
//	ch = WideCharToMultiByte(CP_UTF8, 0, (LPTSTR)gTmpBuf, -1, (LPSTR)gTmpBuf+1, (MAXGBUF-2) * sizeof(TCHAR), NULL, NULL);
//#endif
	if (INVALID_HANDLE_VALUE == (f = CreateFile(GetString(STR_DEBUG_PATH), FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))) return;
	WriteFile(f, gTmpBuf, lstrlen(gTmpBuf)*sizeof(TCHAR), &ch, NULL);
	CloseHandle(f);
}