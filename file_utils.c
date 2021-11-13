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

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#include "include/consts.h"
#include "include/globals.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"



void SetFileFormat(DWORD format, WORD reinterp) {
	HCURSOR hcur;
	HMENU hMenu = GetSubMenu(GetSubMenu(GetMenu(hwnd), 0), MPOS_FILE_FORMAT);
	MENUITEMINFO mio;
	TCHAR mbuf[64], mbuf2[128];
	LPTSTR buf = NULL, pmbuf2 = mbuf2;
	WORD enc, lfmt, cp, nenc, nlfmt, ncp;
	DWORD len, lines, nCR, nLF, nStrays, nSub;
	BOOL bufDirty = FALSE, fileDirty = bDirtyFile, b;

	if (!hMenu || !format) return;
	if (format < 0xffff && format >= FC_LFMT_BASE && format < FC_LFMT_END) format <<= 16;
	if (!(format & 0x8000ffff)) format |= nFormat & 0x8000ffff;
	if (!(format & 0xfff0000)) format |= nFormat & 0xfff0000;
	if (format == nFormat) return;
	enc = cp = (WORD)nFormat;
	nenc = ncp = (WORD)format;
	lfmt = (nFormat >> 16) & 0xfff;
	nlfmt = (format >> 16) & 0xfff;
	if (nFormat >> 31) enc = FC_ENC_CODEPAGE;
	if (format >> 31) nenc = FC_ENC_CODEPAGE;
	if (!GetTextChars(NULL) || 
		( !((enc == FC_ENC_ANSI || enc == FC_ENC_CODEPAGE) && (nenc == FC_ENC_ANSI || nenc == FC_ENC_CODEPAGE) && cp != ncp)
		&& !(lfmt == FC_LFMT_MIXED && nlfmt != lfmt) ) )
		reinterp = 0;
	if (reinterp == 1) {
		if (lfmt != nlfmt)
			pmbuf2 = (LPTSTR)GetString(IDS_LFMT_NORMALIZE);
		else {
			if (nenc == FC_ENC_CODEPAGE)
				PrintCPName(ncp, mbuf, GetString(ID_ENC_CODEPAGE));
			else
				lstrcpy(mbuf, GetString(nenc));
			wsprintf(mbuf2, GetString(IDS_ENC_REINTERPRET), mbuf);
		}
		switch (MessageBox(hwnd, pmbuf2, GetString(STR_METAPAD), MB_YESNOCANCEL | MB_ICONQUESTION)) {
			case IDCANCEL:
				return;
			case IDNO:
				reinterp = 0;
		}
	}
	hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	if (reinterp) {
		RestoreClientView(0, FALSE, TRUE, TRUE);
		buf = (LPTSTR)GetShadowBuffer(&len);
		bLoading = TRUE;
		lines = SendMessage(client, EM_GETLINECOUNT, 0, 0);
		ExportLineFmt(&buf, &len, lfmt, lines, &bufDirty);
		if (cp != ncp)
			len = EncodeText((LPBYTE*)&buf, len, nFormat, &bufDirty, NULL);
		if (len) {
			if (cp != ncp)
				len = DecodeText((LPBYTE*)&buf, len, &format, &bufDirty);
			if (len) {
				GetLineFmt(buf, len, lfmt, &nCR, &nLF, &nStrays, &nSub, &b);
				ImportLineFmt(&buf, &len, nlfmt, nCR, nLF, nStrays, nSub, &bufDirty);
				SetWindowText(client, buf);
				InvalidateRect(client, NULL, TRUE);
			}
		}
		RestoreClientView(0, TRUE, TRUE, TRUE);
		bLoading = FALSE;
		nFormat = format;
		if (!fileDirty && cp != ncp)
			UpdateSavedInfo();
	}
	if (bufDirty)
		FREE(buf);

	nFormat = format;
	DeleteMenu(hMenu, ID_ENC_CODEPAGE, MF_BYCOMMAND);
	if (nenc == FC_ENC_CODEPAGE){
		PrintCPName(ncp, mbuf, GetString(ID_ENC_CODEPAGE));
		mio.cbSize = sizeof(MENUITEMINFO);
		mio.fMask = MIIM_TYPE | MIIM_ID;
		mio.fType = MFT_STRING;
		mio.dwTypeData = mbuf;
		mio.wID = nenc = ID_ENC_CODEPAGE;
		InsertMenuItem(hMenu, GetMenuItemCount(hMenu), TRUE, &mio);
	}
	CheckMenuRadioItem(hMenu, ID_ENC_BASE, ID_ENC_END, nenc % 1000 + ID_MENUCMD_BASE, MF_BYCOMMAND);
	CheckMenuRadioItem(hMenu, ID_LFMT_BASE, ID_LFMT_END, nlfmt % 1000 + ID_MENUCMD_BASE, MF_BYCOMMAND);
	bDirtyShadow = bDirtyStatus = TRUE;
	QueueUpdateStatus();
	SetCursor(hcur);
}

void MakeNewFile(void) {
	bLoading = TRUE;
	SetWindowText(client, _T(""));
	SwitchReadOnly(FALSE);
	FREE(szFile);
	FREE(szCaptionFile);
	SetFileFormat(options.nFormat, 0);
	bLoading = FALSE;
	UpdateSavedInfo();
	UpdateStatus(TRUE);
}

/**
 * Replace '|' with '\0', and adds a '\0' at the end.
 *
 * @param[in] szIn String to fix.
 */
LPTSTR FixFilterString(LPTSTR szIn) {
	LPTSTR sz = szIn;
	INT i = 0;
	while (*++sz)
		if (*sz == _T('|')) {
			*sz = _T('\0');
			i = 1;
		}
	if (!i) return szIn;
	*sz = _T('\0');
	if (*(sz-1)) *++sz = _T('\0');
	return szIn;
}

BOOL FixShortFilename(LPCTSTR szSrc, LPTSTR* szTgt) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hHandle;
	TCHAR dst[MAXFN], ssto;
	LPTSTR pdst = dst, psrc;
	BOOL bOK = TRUE, alloc = FALSE;

	if (!szSrc || !szTgt) return FALSE;
	if (szSrc != *szTgt) alloc = TRUE;
	if (szSrc[0] && szSrc[1] && szSrc[2] != _T('?')) {
		lstrcpy(pdst, _T("\\\\?\\"));
		pdst+=4;
		if (szSrc[0] == _T('\\') && szSrc[1] == _T('\\')) {
			lstrcpy(pdst, _T("UNC"));
			pdst+=3;
			szSrc++;
		}
	}
	while(*szSrc) {
		while(*szSrc == _T('\\'))
			*pdst++ = *szSrc++;
		if (!*szSrc) break;
		pdst[0] = _T('*');
		pdst[1] = _T('\0');
		for (psrc = (LPTSTR)szSrc; *psrc && *psrc != _T('\\'); psrc++) ;
		ssto = *psrc;
		*psrc = _T('\0');
		hHandle = FindFirstFile(dst, &FindFileData);
		bOK = (hHandle != INVALID_HANDLE_VALUE);
		while (bOK && lstrcmpi(FindFileData.cFileName, szSrc) && lstrcmpi(FindFileData.cAlternateFileName, szSrc))
			bOK = FindNextFile(hHandle, &FindFileData);
		if (bOK) lstrcpy(pdst, FindFileData.cFileName);
		else	 lstrcpy(pdst, szSrc);
		*psrc = ssto;
		szSrc = psrc;
		while (*++pdst) ;
		FindClose(hHandle);
	}
	if (alloc) {
		FREE(*szTgt);
		*szTgt = (LPTSTR)HeapAlloc(globalHeap, 0, (pdst-dst+9) * sizeof(TCHAR));
	}
	lstrcpy(*szTgt, dst);
	return bOK;
}
BOOL GetReadableFilename(LPCTSTR lfn, LPTSTR* dst){
	BOOL alloc = FALSE, unc = FALSE;
	if (!lfn || !dst) return FALSE;
	if (lfn != *dst) alloc = TRUE;
	if (lfn[0] && lfn[1] && lfn[2] == _T('?')) {
		lfn += 4;
		if (lfn[0] == _T('U') && lfn[1] == _T('N')) {
			unc = TRUE;
			lfn += 2;
		}
	}
	if (alloc) {
		SSTRCPY(*dst, lfn);
	} else
		*dst = (LPTSTR)lfn;
	if (unc) **dst = _T('\\');
	return TRUE;
}





LPCTSTR GetShadowRange(LONG min, LONG max, LONG line, DWORD* len, CHARRANGE* linecr) {
#ifdef USE_RICH_EDIT
	GETTEXTLENGTHEX gtl = {0};
	TEXTRANGE tr;
#endif
	DWORD l;
	CHARRANGE lcr = {0};
	if (len) *len = 0;
	if (linecr) *linecr = lcr;
	if (max <= min && max >= 0) return _T("");
	if (bLoading) return szShadow;
	if (bDirtyShadow || !szShadow || !shadowLen || shadowLine >= 0) {
#ifdef USE_RICH_EDIT
		l = shadowLen = SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
#else
		l = shadowLen = GetWindowTextLength(client);
#endif
		if (l < 1) return _T("");
		if (l+9 > shadowAlloc || (shadowAlloc / 4 > l+9)) {
			printf("\nA!");
			shadowAlloc = ((l+9) / 2) * 3;
			if (szShadow) {
				szShadow -= 8;
				FREE(szShadow);
			}
			szShadow = (LPTSTR)HeapAlloc(globalHeap, 0, shadowAlloc * sizeof(TCHAR));
			if (!szShadow) {
				ReportLastError();
				return _T("");
			}
			szShadow += 8;	//The extra leading bytes are used in: EditProc() -> case WM_CHAR (autoindent newline insertion)
		}
		if (line < 0 && min <= max && max >= 0) {
#ifdef USE_RICH_EDIT
			line = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)min);
			if (line != SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)max)) line = -1;
#else
			line = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)min, 0);
			if (line != SendMessage(client, EM_LINEFROMCHAR, (WPARAM)max, 0)) line = -1;
#endif
		}
		if (line >= 0) {
			lcr.cpMin = SendMessage(client, EM_LINEINDEX, (WPARAM)line, 0);
			lcr.cpMax = lcr.cpMin + SendMessage(client, EM_LINELENGTH, (WPARAM)lcr.cpMin, 0);
			l = lcr.cpMax - lcr.cpMin;
			if (linecr) *linecr = lcr;
			if (max < min) { min = lcr.cpMin; max = lcr.cpMax; }
		}
		if (l < 1) return _T("");
		if (line >= 0) {
			if (bDirtyShadow || shadowLine != line) {
				printf("l");
				shadowRngEnd = 0;
				*((LPWORD)(szShadow+lcr.cpMin)) = (USHORT)l;
				l = SendMessage(client, EM_GETLINE, (WPARAM)line, (LPARAM)(LPCTSTR)(szShadow+lcr.cpMin));
				szShadow[lcr.cpMin+l] = 0;
			}
		} else {
			printf("G");
			shadowRngEnd = 0;
#ifdef USE_RICH_EDIT
			tr.chrg.cpMin = 0;
			tr.chrg.cpMax = -1;
			tr.lpstrText = szShadow;
			SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
#else
			GetWindowText(client, szShadow, shadowLen+1);
#endif
		}
		shadowLine = line;
		bDirtyShadow = FALSE;
	} else if (line >= 0) {
		lcr.cpMin = SendMessage(client, EM_LINEINDEX, (WPARAM)line, 0);
		lcr.cpMax = lcr.cpMin + SendMessage(client, EM_LINELENGTH, (WPARAM)lcr.cpMin, 0);
		l = lcr.cpMax - lcr.cpMin;
		if (linecr) *linecr = lcr;
		if (max < min) { min = lcr.cpMin; max = lcr.cpMax; }
	}
	printf(".");
	if (min < 0) min = 0;
	if (max < 0) l = shadowLen;
	else l = MIN((DWORD)(max-min), shadowLen);
	if (min + l != shadowRngEnd){
		if (shadowRngEnd)
			szShadow[shadowRngEnd] = shadowHold;
		shadowRngEnd = min + l;
		if (shadowRngEnd >= shadowLen)
			shadowRngEnd = 0;
		else {
			shadowHold = szShadow[shadowRngEnd];
			szShadow[shadowRngEnd] = _T('\0');
		}
	}
	if (len) *len = l;
	return szShadow + min;
}
LPCTSTR GetShadowSelection(DWORD* len, CHARRANGE* pcr) {
	CHARRANGE cr;
#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
	if (pcr) *pcr = cr;
	return GetShadowRange(cr.cpMin, cr.cpMax, -1, len, NULL);
}
LPCTSTR GetShadowLine(LONG line, LONG cp, DWORD* len, LONG* lineout, CHARRANGE* pcr){
	if (line < 0) {
#ifdef USE_RICH_EDIT
		line = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)MAX(cp, -1));
#else
		line = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)MAX(cp, -1), 0);
#endif
	}
	if (lineout) *lineout = line;
	return GetShadowRange(0, -1, line, len, pcr);
}
LPCTSTR GetShadowBuffer(DWORD* len) {
	return GetShadowRange(0, -1, -1, len, NULL);
}



DWORD GetColNum(LONG cp, LONG line, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr){
	LPCTSTR ts;
	CHARRANGE cr;
	DWORD c = 1, i, l;
	if (cp < 0) {
#ifdef USE_RICH_EDIT
		SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
		SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
		cp = cr.cpMax;
		if (pcr) *pcr = cr;
	}
	ts = GetShadowLine(line, cp, lineLen, lineout, &cr);
	for (i = 0, l = cp - cr.cpMin; i < l && ts[i]; i++, c++) {
		if (ts[i] == _T('\t'))
			c += (options.nTabStops - (c-1) % options.nTabStops)-1;
#ifdef UNICODE
		else if (ts[i] >= _T('\xd800') && ts[i] < _T('\xdc00'))	//Unicode surrogate pair - displays as 1 char while actually taking up 2 in memory
			c--;
#endif
	}
	return c;
}
DWORD GetCharIndex(DWORD col, LONG line, LONG cp, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr){
	LPCTSTR ts;
	DWORD c = 1, i;
	ts = GetShadowLine(line, cp, lineLen, lineout, pcr);
	for (i = 0; ts[i] && ts[i] != _T('\r') && c < col; i++, c++) {
		if (ts[i] == _T('\t'))
			c += (options.nTabStops - (c-1) % options.nTabStops)-1;
#ifdef UNICODE
		else if (ts[i] >= _T('\xd800') && ts[i] < _T('\xdc00'))	//Unicode surrogate pair - displays as 1 char while actually taking up 2 in memory
			c--;
#endif
	}
	return i;
}



DWORD GetTextChars(LPCTSTR szText){
	//This must exactly mirror the text length calculation in GetShadowRange() 
#ifdef USE_RICH_EDIT
	GETTEXTLENGTHEX gtl = {0};
	//if (lfmt != FC_LFMT_UNIX && lfmt != FC_LFMT_MAC) gtl.flags = GTL_USECRLF;
	if (!szText) return SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
#else
	if (!szText) return GetWindowTextLength(client);
#endif
	else return lstrlen(szText);
}
/**
 * Calculate the size of the given text.
 * If no text is given, the current file's size is calculated and the full buffer is optionally returned
 * 	if getting it was necessary in the calculation
 */
DWORD CalcTextSize(LPCTSTR* szText, CHARRANGE* range, DWORD estBytes, DWORD format, BOOL inclBOM, DWORD* numChars) {
	LPCTSTR szBuffer = (szText ? *szText : NULL);
	DWORD chars, mul = 1, bom = 0;
	WORD enc, cp = CP_ACP;
	BOOL deep = FALSE, urange = FALSE;
	if (format >> 31) {
		enc = FC_ENC_CODEPAGE;
		cp = (WORD)format;
	} else enc = (WORD)format;
	if (range && range->cpMin <= range->cpMax && range->cpMin >= 0 && range->cpMax >= 0) urange = TRUE;
	if (!estBytes && urange) estBytes = range->cpMax - range->cpMin;
	if (enc == FC_ENC_UTF8 || enc == FC_ENC_CODEPAGE || ExportLineFmtDelta(NULL, NULL, (format >> 16) & 0xfff)){
		deep = TRUE;
		if (!szBuffer) {
			if (urange) szBuffer = GetShadowRange(range->cpMin, range->cpMax, -1, &estBytes, NULL);
			else szBuffer = GetShadowBuffer(&estBytes);
		} else if (!estBytes && !urange) estBytes = lstrlen(szBuffer);
	} else if (!estBytes && !urange)
		estBytes = GetTextChars(szBuffer);
	chars = estBytes;
	if (deep && estBytes)
		ExportLineFmtDelta(szBuffer, &estBytes, (format >> 16) & 0xfff);
	if (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE) {
		mul = bom = 2;
	} else if (enc == FC_ENC_UTF8 || enc == FC_ENC_CODEPAGE) {
		if (enc == FC_ENC_UTF8) {
			bom = 3;
			cp = CP_UTF8;
		}
#ifdef UNICODE
		if (estBytes)
			estBytes = WideCharToMultiByte(cp, 0, szBuffer, estBytes, NULL, 0, NULL, NULL);
#endif
	}
	if (szText) *szText = szBuffer;
	if (numChars) *numChars = chars;
	return estBytes * mul + (inclBOM ? bom : 0);
}



CHARRANGE GetSelection() {
	CHARRANGE cr;
#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
	return cr;
}
LRESULT SetSelection(CHARRANGE cr) {
#ifdef USE_RICH_EDIT
	return SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
	return SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
}
void RestoreClientView(WORD index, BOOL restore, BOOL sel, BOOL scroll){
	static LONG x[8], y[8];
	static CHARRANGE cr[8];
	if (restore) {
		if (sel) SetSelection(cr[index]);
		if (scroll) {
			SendMessage(client, EM_SCROLLCARET, 0, 0);
			if (x[index] < 0x10000) SendMessage(client, WM_HSCROLL, SB_THUMBPOSITION | (x[index]<<16), 0);
			if (y[index] < 0x10000) SendMessage(client, WM_VSCROLL, SB_THUMBPOSITION | (y[index]<<16), 0);
		}
	} else {
		if (sel) cr[index] = GetSelection();
		if (scroll) {
			y[index] = GetScrollPos(client, SB_VERT);
			x[index] = GetScrollPos(client, SB_HORZ);
		}
	}
}
BOOL IsSelectionVisible(){
	RECT rc;
	POINT pt1, pt2;
	CHARRANGE cr = GetSelection();
	GetWindowRect(client, &rc);
	cr.cpMin = MAX(cr.cpMin-1, 0);
	cr.cpMax = MAX(cr.cpMax-1, 0);
#ifdef USE_RICH_EDIT
	SendMessage(client, EM_POSFROMCHAR, (WPARAM)&pt1, cr.cpMin);
	SendMessage(client, EM_POSFROMCHAR, (WPARAM)&pt2, cr.cpMax);
#else
	pt1.y = SendMessage(client, EM_POSFROMCHAR, cr.cpMin, 0) >> 16;
	pt2.y = SendMessage(client, EM_POSFROMCHAR, cr.cpMax, 0) >> 16;
#endif
	return pt1.y < HEIGHT(rc)-16 && pt2.y >= 0;
}





void UpdateSavedInfo() {
	DWORD lChars, l, m;
	LPCTSTR buf;
	bDirtyFile = FALSE;
	bDirtyShadow = bDirtyStatus = TRUE;
	buf = GetShadowBuffer(&lChars);
	savedChars = lChars;
	savedFormat = nFormat & 0x8fffffff;
	memset(savedHead, 0, sizeof(savedHead));
	memset(savedFoot, 0, sizeof(savedFoot));
	memcpy(savedHead, (LPBYTE)buf, l = MIN(m = (lChars*sizeof(TCHAR)), sizeof(savedHead)));
	memcpy(savedFoot, ((LPBYTE)(buf + lChars)) - l, l);
	if (m > 64) EvaHash(((LPBYTE)buf)+32, m-64, savedHash);
}




CHARRANGE DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop, LPBYTE pbFindSpec) {
	LONG lSize;
	LPCTSTR szBuffer = GetShadowBuffer(&lSize);
	LPCTSTR lpszStop, lpsz, lpfs = NULL, lpszFound = NULL;
	DWORD cf = 0, nFindLen = lstrlen(szText), cg = 0, lg = 0, k, m = 1;
	TCHAR gc;
	CHARRANGE r;

	r.cpMin = -1;
	r.cpMax = -1;
	if (!szBuffer) {
		ReportLastError();
		return r;
	}
	if (bDown) {
		lpszStop = szBuffer + lSize;
		lpsz = szBuffer + (bFromTop ? 0 : lStart + (lStart == lEnd ? 0 : 1));
	} else {
		lpszStop = szBuffer + (bFromTop ? lSize : lStart);
		lpsz = szBuffer;
	}

	while( lpszStop != szBuffer && lpsz - (bDown? 0 : cf) < lpszStop && (!bDown || (bDown && lpszFound == NULL)) ) {
		if ( m && ((pbFindSpec && (k = pbFindSpec[cf]) && (k < 4 || (cf && k < 6 && *lpsz == gc) || (k == 6 && !(RAND()%0x60)))) 
			|| *lpsz == szText[cf] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*lpsz) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szText[cf])) )) {
			if (pbFindSpec){
				if (k && k < 6) {
					if ((k == 2 || (lg && k == 3)) && cf+1 < nFindLen && (*lpsz == szText[cf+1] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*lpsz) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szText[cf+1])))) {
						cg = ++cf; continue;
					} else if (k != 1) {
						lg++;
						if (lpsz+1 != lpszStop) cf--;
						else m = 2;
					}
				} else {
					lg = 0; gc = *lpsz;
				}
			}
			if (!lpfs) lpfs = lpsz;
			if (++cf == nFindLen){
				if (!bWholeWord || ( (lpfs == szBuffer || !(_istalnum(*(lpfs-1)) || *(lpfs-1) == _T('_'))) && !(*(lpsz+1) && (_istalnum(*(lpsz+1)) || *(lpsz+1) == _T('_'))) )) {
					lpszFound = lpfs;
					r.cpMin = lpfs - szBuffer;
					r.cpMax = lpsz - szBuffer + 1;
					m = 1;
				} else if (cg) { 
					m = 0; continue; 
				}
				if (!bDown) lpsz = lpfs;
				lpfs = NULL;
				cf = cg = lg = 0;
			}
		} else if (lpfs) {
			if (pbFindSpec){
				m = 2;
				if ((lg || k == 2 || k == 4) && cf+1 < nFindLen && (*lpsz == szText[cf+1] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*lpsz) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szText[cf+1]))))
					cf++;
				else if (cg) { cf = cg-1; lg=1; }
				else if (!((k == 4 || (lg && k == 5)) && ++cf)) m = 1;
			}
			if (m < 2) {
				lpfs = NULL;
				cf = lg = 0;
			}
			m = 1;
			continue;
		}
		lpsz++;
		if (m == 2) {
			m = 1;
			lpsz = ++lpfs;
			cf = cg = lg = 0;
		}
	}
	return r;
}

BOOL SearchFile(LPCTSTR szText, BOOL bCase, BOOL bDown, BOOL bWholeWord, LPBYTE pbFindSpec) {
	CHARRANGE f1, f2;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	CHARRANGE cr = GetSelection();
	f1 = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, FALSE, pbFindSpec);
	if (f1.cpMin < 0) {
		f2 = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, TRUE, pbFindSpec);
		if (f2.cpMin >= 0) {
			if (!options.bFindAutoWrap && MessageBox(hdlgFind ? hdlgFind : client, bDown ? GetString(IDS_QUERY_SEARCH_TOP) : GetString(IDS_QUERY_SEARCH_BOTTOM), GetString(STR_METAPAD), MB_OKCANCEL|MB_ICONQUESTION) == IDCANCEL) {
				SetCursor(hcur);
				return FALSE;
			}
			else if (options.bFindAutoWrap) MessageBeep(MB_OK);
			f1 = f2;
		}
	}
	SetCursor(hcur);
	if (f1.cpMin >= 0) {
		SendMessage(client, EM_SETSEL, (WPARAM)f1.cpMin, (LPARAM)f1.cpMax);
		QueueUpdateStatus();
		return TRUE;
	}
	MessageBox(hdlgFind ? hdlgFind : client, GetString(IDS_ERROR_SEARCH), GetString(STR_METAPAD), MB_OK|MB_ICONINFORMATION);
	return FALSE;
}





DWORD StrReplace(LPCTSTR szIn, LPTSTR* szOut, DWORD* bufLen, LPCTSTR szFind, LPCTSTR szRepl, LPBYTE pbFindSpec, LPBYTE pbReplSpec, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen){
	//0123456
	// _?*-+$
	DWORD k, m = 1, len, alen, ilen, lf, lr, ct = 0, cf = 0, cg = 0, lg = 0, nglob[6] = {0}, cglob[6] = {0}, sglob[6], gu = 0;
	WORD enc = (nFormat >> 31 ? FC_ENC_CODEPAGE : (WORD)nFormat);
	LPTSTR dst, odst, pd = NULL;
	LPCTSTR *globs[6], *globe[6];
	TCHAR gc;
	if (!szIn || !szFind || !*szFind) return ct;
	ilen = (len = alen = (bufLen && *bufLen >= 0 ? *bufLen : lstrlen(szIn))) + 1;
	if (len < 1) return ct;
	dst = odst = (LPTSTR)HeapAlloc(globalHeap, 0, (alen+1) * sizeof(TCHAR));
	szRepl = SCNUL(szRepl);
	lf = lstrlen(szFind);
	lr = lstrlen(szRepl);
	if (pbFindSpec && pbReplSpec) {
		for (k = 0; k < lf; k++) { if (pbFindSpec[k] < 6) nglob[pbFindSpec[k]]++; }
		for (k = 0; k < lr; k++) { if (pbReplSpec[k] < 6) cglob[pbReplSpec[k]]++; }
		for (k = 0; ++k < 6; ) {
			globs[k] = globe[k] = NULL;
			if (nglob[k] = MIN(nglob[k], cglob[k])) {
				globs[k] = (LPCTSTR*)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nglob[k] * sizeof(LPCTSTR*));
				globe[k] = (LPCTSTR*)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nglob[k] * sizeof(LPCTSTR*));
				gu = 1;
			}
		}
		memset(cglob, 0, sizeof(cglob));
	}
	while (--ilen) {
		if ( m && ((pbFindSpec && (k = pbFindSpec[cf]) && (k < 4 || (cf && k < 6 && *szIn == gc) || (k == 6 && !(RAND()%0x60))))
			|| (*szIn == szFind[cf] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*szIn) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szFind[cf]))) )) {
			if (pbFindSpec) {
				if (k && k < 6) {
					if (gu && nglob[k] && cglob[k] <= nglob[k]) {
						if (!lg && ++cglob[k] <= nglob[k])
							globs[k][cglob[k]-1] = szIn;
						if (cglob[k] <= nglob[k]) 
							globe[k][cglob[k]-1] = szIn + (k == 1 ? 1 : 0);
					}
					if ((k == 2 || (lg && k == 3)) && cf+1 < lf && (*szIn == szFind[cf+1] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*szIn) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szFind[cf+1])))) {
						cg = ++cf;
						memcpy(sglob, cglob, sizeof(cglob));
						ilen++;
						continue;
					} else if (k != 1) { 
						lg++;
						if (ilen-1) cf--;
						else {
							m = 2;
							if (gu && nglob[k] && cglob[k] <= nglob[k])
								globe[k][cglob[k]-1] = szIn + 1;
						} 
					}
				} else {
					lg = 0; gc = *szIn;
				}
			}
			if (!pd) pd = dst;
			if (++cf == lf || (matchLen && maxLen && (DWORD)(dst-pd+1) >= maxLen)) {
				if ((!maxLen || (DWORD)(dst-pd) < maxLen) && (!bWholeWord || ( (pd == odst || !(_istalnum(*(pd-1)) || *(pd-1) == _T('_'))) && !(*(szIn+1) && (_istalnum(*(szIn+1)) || *(szIn+1) == _T('_'))) ))) {
					len += (pd - dst) + lr - 1;
					GROWBUF(odst, dst, len, alen, pd, LPTSTR, TCHAR);
					if (pbReplSpec) {
						if (gu) memset(cglob, 0, sizeof(cglob));
						for(cf = 0, lg = 0, dst = pd; cf < lr; cf++, lg++){
							if (gu && (k = pbReplSpec[cf]) && k < 6 && nglob[k]) {
								if (globs[k][cglob[k]%nglob[k]])
									lg += (cg = (globe[k][cglob[k]%nglob[k]] - globs[k][cglob[k]%nglob[k]])) - 1;
								else {
									globs[k][cglob[k]%nglob[k]] = _T("");
									lg--; cg = 0;
								}
								GROWBUF(odst, dst, (len + lg), alen, pd, LPTSTR, TCHAR);
								lstrcpyn(pd, globs[k][cglob[k]++%nglob[k]], cg + 1);
								pd += cg;
							} else if (pbReplSpec[cf] == 6) {
#ifdef UNICODE
								if (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE)
									*pd++ = RAND()%0xffe0+0x20;
								else
#endif
									*pd++ = RAND()%0x60+0x20;
							} else
								*pd++ = szRepl[cf];
						}
						len += lg - lr;
						dst = pd;
					} else {
						lstrcpyn(pd, szRepl, lr + 1);
						dst = pd + lr;
					}
					m = 1;
				} else if (cg) {
					m = 0; ilen++; continue;
				}
				pd = NULL;
				cf = cg = lg = 0;
				if (gu) { 
					memset(cglob, 0, sizeof(cglob));		
					for (k = 0; ++k < 6; ) {
						memset((LPTSTR*)globs[k], 0, nglob[k] * sizeof(LPCTSTR*));
						memset((LPTSTR*)globe[k], 0, nglob[k] * sizeof(LPCTSTR*));
					}
				}
				szIn++;
				if (++ct >= maxMatch && maxMatch) { ilen--; break; }
				continue;
			}
		} else if (pd) {
			if (pbFindSpec){
				m = 2;
				if ((lg || k == 2 || k == 4) && cf+1 < lf && (*szIn == szFind[cf+1] || (!bCase && (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)*szIn) == (TCHAR)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)(BYTE)szFind[cf+1])))) {
					cf++;
					if (gu && nglob[k] && cglob[k] <= nglob[k])
						globe[k][cglob[k]-1] = szIn;
				} else if (cg) {
					cf = cg-1;
					lg = 1;
					memcpy(cglob, sglob, sizeof(cglob));
				} else if ((k == 4 || (lg && k == 5)) && ++cf){
					if (gu && nglob[k] && cglob[k] <= nglob[k]) {
						if (!lg && ++cglob[k] <= nglob[k])
							globs[k][cglob[k]-1] = szIn;
						if (cglob[k] <= nglob[k]) 
							globe[k][cglob[k]-1] = szIn;
					}
					lg = 0;
				} else 
					m = 1;
			}
			if (m < 2) {
				pd = NULL;
				cf = lg = 0;
				if (gu) { 
					memset(cglob, 0, sizeof(cglob));		
					for (k = 0; ++k < 6; ) {
						memset((LPTSTR*)globs[k], 0, nglob[k] * sizeof(LPCTSTR*));
						memset((LPTSTR*)globe[k], 0, nglob[k] * sizeof(LPCTSTR*));
					}
				}
			}
			m = 1;
			ilen++;
			continue;
		}
		if (m == 2){
			m = 1;
			szIn -= (dst - pd);
			ilen += (dst - pd);
			dst = ++pd;
			continue;
		}
		*dst++ = *szIn++;
	}
	for ( ; ilen--; *dst++ = *szIn++ ) ;
	*dst = _T('\0');
	if (bufLen) *bufLen = len;
	if (szOut) {
		FREE(*szOut);
		*szOut = odst;
	}
	if (gu)
		for (k = 0; ++k < 6; ) {
			FREE(globs[k]);
			FREE(globe[k]);
		}
	return ct;
}

DWORD ReplaceAll(HWND owner, DWORD nOps, DWORD recur, LPCTSTR* szFind, LPCTSTR* szRepl, LPBYTE* pbFindSpec, LPBYTE* pbReplSpec, LPTSTR szMsgBuf, BOOL selection, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen, LPCTSTR header, LPCTSTR footer){
	HCURSOR hCur;
	LPCTSTR szIn;
	LPTSTR szBuf = NULL, szTmp = NULL;
	CHARRANGE cr;
	DWORD l, r = 0, lh = 0, lf = 0, ict = 0, nr, gr, op;

	if (!szFind || !nOps) return 0;
	if (!owner) owner = hwnd;
	if (header) lh = lstrlen(header);
	if (footer) lf = lstrlen(footer);
	hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	if (selection){
		szIn = GetShadowSelection(&l, &cr);
		if (!l) {
			if (owner) MessageBox(owner, GetString(IDS_NO_SELECTED_TEXT), GetString(STR_METAPAD), MB_OK|MB_ICONINFORMATION);
			return 0;
		}
	} else
		szIn = GetShadowBuffer(&l);
	if (lh || lf) {
		szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, (l + lh + lf + 1) * sizeof(TCHAR));
		if (lh) lstrcpy(szTmp, header);
		lstrcpy(szTmp + lh, szIn);
		if (lf) lstrcpy(szTmp+lh+l, footer);
		szIn = (LPCTSTR)szTmp;
	}
	l += lh+lf;
	do {
		gr = 0;
		for (op = 0; op < nOps; op++){
			do {
				r += (nr = StrReplace(szIn, &szBuf, &l, szFind[op], szRepl ? szRepl[op] : NULL, pbFindSpec ? pbFindSpec[op] : NULL, pbReplSpec ? pbReplSpec[op] : NULL, bMatchCase, bWholeWord, maxMatch, maxLen, matchLen));
				maxMatch -= r;
				gr += nr;
				ict++;
				szIn = (LPCTSTR)szBuf;
			} while ((recur & 1) && nr);
		}
	} while ((recur & 2) && gr);
	if (r) {
		SendMessage(client, WM_SETREDRAW, (WPARAM)FALSE, 0);
		if (!selection) {
			cr.cpMin = 0; cr.cpMax = -1;
			SetSelection(cr);
		}
		szBuf[l - lf] = _T('\0');
		SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(szBuf+lh));
		if (selection) {
			cr.cpMax = cr.cpMin + l - lf;
			SetSelection(cr);
		}
	}
	FREE(szTmp);
	FREE(szBuf);
	SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hCur);
	if (owner && szMsgBuf) {
		wsprintf(szMsgBuf, GetString(recur ? IDS_ITEMS_REPLACED_ITER : IDS_ITEMS_REPLACED), r, ict-1);
		MessageBox(owner, szMsgBuf, GetString(STR_METAPAD), MB_OK|MB_ICONINFORMATION);
	}
	return r;
}

