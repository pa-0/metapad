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

#ifdef USE_RICH_EDIT
#include <richedit.h>
#include <commdlg.h>
#endif

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"



void SetFileFormat(int nFormat) {
/*	switch (nFormat) {
	case FILE_FORMAT_DOS: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_DOS_FILE, 0), 0); break;
	case FILE_FORMAT_UNIX: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNIX_FILE, 0), 0); break;
	case FILE_FORMAT_UTF8: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UTF8_FILE, 0), 0); break;
	case FILE_FORMAT_UTF8_UNIX: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UTF8_UNIX_FILE, 0), 0); break;
	case FILE_FORMAT_UNICODE: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNICODE_FILE, 0), 0); break;
	case FILE_FORMAT_UNICODE_BE: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_UNICODE_BE_FILE, 0), 0); break;
	case FILE_FORMAT_BINARY: SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BINARY_FILE, 0), 0); break;
	}*/
}

void MakeNewFile(void) {
	bLoading = TRUE;
	SetFileFormat(options.nFormat);
	SetWindowText(client, _T(""));
	bDirtyFile = FALSE;
	bLoading = FALSE;
	SwitchReadOnly(FALSE);
	FREE(szFile);
	SSTRCPYAO(szCaptionFile, GetString(IDS_NEW_FILE), 32, 8);
	szCaptionFile[0] = _T('\0');
	bDirtyShadow = bDirtyStatus = TRUE;
	savedChars = 0;
	savedFormat = nFormat;
	UpdateStatus(TRUE);
	bLoading = FALSE;
	//TODO
}

int FixShortFilename(LPCTSTR szSrc, TCHAR *szDest)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hHandle;
	TCHAR sDir[MAXFN], sName[MAXFN];
	int nDestPos=0, nSrcPos=0, i;
	BOOL bOK = TRUE;

	// Copy drive letter over
	if (szSrc[1] == _T(':')) {
		szDest[nDestPos++] = szSrc[nSrcPos++];
		szDest[nDestPos++] = szSrc[nSrcPos++];
	}

	while (szSrc[nSrcPos]) {
		// If the next TCHAR is '\' we are starting from the root and want to add '\*' to sDir.
		// Otherwise we are doing relative search, so we just append '*' to sDir
		if (szSrc[nSrcPos]==_T('\\')) {
			szDest[nDestPos++] = szSrc[nSrcPos++];

			if (szSrc[nSrcPos] == _T('\\')) { // get UNC server name
				szDest[nDestPos++] = szSrc[nSrcPos++];

				while (szSrc[nSrcPos] && szSrc[nSrcPos - 1]!=_T('\\')) {
					szDest[nDestPos++] = szSrc[nSrcPos++];
				}
			}
		}

		_tcsncpy(sDir, szDest, nDestPos);
		sDir[nDestPos] = _T('*');
		sDir[nDestPos + 1] = _T('\0');

		for (i=0; szSrc[nSrcPos] && szSrc[nSrcPos]!=_T('\\'); i++)
			sName[i] = szSrc[nSrcPos++];
		sName[i] = _T('\0');

		hHandle = FindFirstFile(sDir, &FindFileData);
		bOK = (hHandle != INVALID_HANDLE_VALUE);
		while (bOK && lstrcmpi(FindFileData.cFileName, sName) != 0 && lstrcmpi(FindFileData.cAlternateFileName, sName) != 0)
			bOK = FindNextFile(hHandle, &FindFileData);

    	if (bOK)
    		_tcscpy(&szDest[nDestPos], FindFileData.cFileName);
    	else
    		_tcscpy(&szDest[nDestPos], sName);

		// Fix the length of szDest
		nDestPos = _tcslen(szDest);
		if (hHandle)
			FindClose(hHandle);
	}
	return !bOK;
}

void ExpandFilename(LPCTSTR szBuffer, LPTSTR* szOut)
{
	WIN32_FIND_DATA FileData;
	HANDLE hSearch;
	TCHAR szTmp[MAXFN+6] = _T("\\\\?\\");
	LPTSTR szTmpFn = szTmp+4;
	LPTSTR szTmpDir;

	if (!szBuffer) return;
	lstrcpy(szTmpFn, szBuffer);
	FixShortFilename(szBuffer, szTmpFn);
	szBuffer = szTmpFn;
	if (szOut) SSTRCPY(*szOut, szBuffer);
	if (SCNUL(szDir)[0] != _T('\0'))
		SetCurrentDirectory(szDir);

	hSearch = FindFirstFile(szTmp, &FileData);
	FREE(szCaptionFile);
	if (hSearch != INVALID_HANDLE_VALUE) {
		LPCTSTR pdest;
		szTmpDir = (LPTSTR)HeapAlloc(globalHeap, 0, (MAX(lstrlen(szBuffer)+2, lstrlen(SCNUL(szDir))+1)) * sizeof(TCHAR));
		lstrcpy(szTmpDir, SCNUL(szDir));
		pdest = _tcsrchr(szBuffer, _T('\\'));
		if (pdest) {
			int result;
			result = pdest - szBuffer + 1;
			lstrcpyn(szTmpDir, szBuffer, result);
		}
		if (szTmpDir[lstrlen(szTmpDir) - 1] != _T('\\'))
			lstrcat(szTmpDir, _T("\\"));
		szCaptionFile = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(szTmpDir)+lstrlen(FileData.cFileName)+40) * sizeof(TCHAR));
		szCaptionFile[0] = szCaptionFile[8] = _T('\0');
		if (!options.bNoCaptionDir)
			lstrcat(szCaptionFile+8, szTmpDir);
		lstrcat(szCaptionFile+8, FileData.cFileName);
		FindClose(hSearch);
		if (szDir) HeapFree(globalHeap, 0, (HGLOBAL)szDir);
		szDir = szTmpDir;
	} else {
		int i = (lstrlen(SCNUL(szDir))+lstrlen(SCNUL(szFile))+1) * sizeof(TCHAR);
		szCaptionFile = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(SCNUL(szDir))+lstrlen(SCNUL(szFile))+40) * sizeof(TCHAR));
		szCaptionFile[0] = szCaptionFile[8] = _T('\0');
		if (!options.bNoCaptionDir)
			lstrcat(szCaptionFile+8, SCNUL(szDir));
		lstrcat(szCaptionFile+8, SCNUL(szFile));
	}
}






LPCTSTR GetShadowRange(LONG min, LONG max, LONG line, DWORD* len) {
	DWORD l;
	if (max <= min && max >= 0){
		if (len) *len = 0;
		return _T("");
	}
	if (bDirtyShadow || !szShadow || !shadowLen) {
		if (line >= 0 && (l = max - min + 1) > 0)
			shadowLen = 0;
		else {
#ifdef USE_RICH_EDIT
			GETTEXTLENGTHEX gtl = {0};
			l = shadowLen = SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
#else
			l = shadowLen = GetWindowTextLength(client);
#endif
			shadowRngEnd = 0; 
		}
		if (l < 1) {
			if (len) *len = 0;
			return _T("");
		}
		if (l + 9 > shadowAlloc || (line < 0 && shadowAlloc / 4 > l)) {
			printf("\nA!");
			shadowAlloc = ((l + 9) / 2) * 3;
			if (szShadow) {
				szShadow -= 8;
				FREE(szShadow);
			}
			szShadow = (LPTSTR)HeapAlloc(globalHeap, 0, shadowAlloc * sizeof(TCHAR));
			if (!szShadow) {
				ReportLastError();
				return _T("");
			}
			szShadow += 8;
		}
		if (line >= 0) {
			if (bDirtyShadow || shadowRngEnd != line) {
				printf("l");
				shadowRngEnd = line;
				*((LPWORD)szShadow) = (USHORT)(l + 1);
				SendMessage(client, EM_GETLINE, (WPARAM)line, (LPARAM)(LPCTSTR)szShadow);
				szShadow[l-1] = '\0';
				bDirtyShadow = FALSE;
			}
			if (len) *len = l;
			return szShadow;
		} else {
			printf("G");
#ifdef USE_RICH_EDIT
			{
				TEXTRANGE tr;
				tr.chrg.cpMin = 0;
				tr.chrg.cpMax = -1;
				tr.lpstrText = szShadow;
				SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
			}
#else
			GetWindowText(client, szShadow, shadowLen+1);
#endif
			bDirtyShadow = FALSE;
		}
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
	return GetShadowRange(cr.cpMin, cr.cpMax, -1, len);
}
LPCTSTR GetShadowLine(LONG line, LONG cp, DWORD* len, LONG* lineout, CHARRANGE* pcr){
	CHARRANGE cr;
	if (line < 0) {
#ifdef USE_RICH_EDIT
		line = SendMessage(client, EM_EXLINEFROMCHAR, 0, (LPARAM)MAX(cp, -1));
#else
		line = SendMessage(client, EM_LINEFROMCHAR, (WPARAM)MAX(cp, -1), 0);
#endif
	}
	cr.cpMin = SendMessage(client, EM_LINEINDEX, (WPARAM)line, 0);
	cr.cpMax = cr.cpMin + SendMessage(client, EM_LINELENGTH, (WPARAM)cr.cpMin, 0);
	if (pcr) *pcr = cr;
	if (lineout) *lineout = line;
	return GetShadowRange(cr.cpMin, cr.cpMax, line, len);
}
LPCTSTR GetShadowBuffer(DWORD* len) {
	return GetShadowRange(0, -1, -1, len);
}



DWORD GetColNum(LONG cp, LONG line, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr){
	LPCTSTR ts;
	CHARRANGE cr;
	DWORD c = 1, i, l;
	ts = GetShadowLine(line, cp, lineLen, lineout, &cr);
	for (i = 0, l = cp - cr.cpMin; i < l && ts[i]; i++, c++) {
		if (ts[i] == _T('\t'))
			c += (options.nTabStops - (c-1) % options.nTabStops)-1;
	}
	if (pcr) *pcr = cr;
	return c;
}
DWORD GetCharIndex(DWORD col, LONG line, LONG cp, DWORD* lineLen, LONG* lineout, CHARRANGE* pcr){
	LPCTSTR ts;
	DWORD c = 1, i;
	ts = GetShadowLine(line, cp, lineLen, lineout, pcr);
	for (i = 0; ts[i] && ts[i] != _T('\r') && c < col; i++, c++) {
		if (ts[i] == _T('\t'))
			c += (options.nTabStops - (c-1) % options.nTabStops)-1;
	}
	return i;
}



/**
 * Calculate the size of the given text.
 * If no text is given, the current file's size is calculated and the full buffer is optionally returned
 * 	if getting it was necessary in the calculation
 */
DWORD CalcTextSize(LPCTSTR* szText, DWORD estBytes, WORD encoding, BOOL unix, BOOL inclBOM, DWORD* numChars) {
	LPCTSTR szBuffer = (szText ? *szText : NULL);
	DWORD chars, mul = 1, bom = 0;
#ifdef USE_RICH_EDIT
	GETTEXTLENGTHEX gtl = {0};
	if (!unix) gtl.flags = GTL_USECRLF;
	unix = FALSE;
	if (!szBuffer) estBytes = SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
#else
	if (!szBuffer && !unix) estBytes = GetWindowTextLength(client);
#endif
	else if (!estBytes) estBytes = lstrlen(szBuffer);
	chars = estBytes;
/*	if (encoding == TYPE_UTF_16 || encoding == TYPE_UTF_16_BE) {
		mul = 2;
		bom = SIZEOFBOM_UTF_16;
	} else if (encoding == TYPE_UTF_8) {
		if (!szBuffer) szBuffer = GetShadowBuffer(&estBytes);
		chars = estBytes;
#ifdef UNICODE
		if (estBytes)
			estBytes = WideCharToMultiByte(CP_UTF8, 0, szBuffer, estBytes, NULL, 0, NULL, NULL);
#endif
		bom = SIZEOFBOM_UTF_8;
	}*/
#ifndef USE_RICH_EDIT
	if (unix) {
		if (!szBuffer) szBuffer = GetShadowBuffer(&estBytes);
		chars = estBytes;
		for ( ; *szBuffer; szBuffer++)
			if (*szBuffer == _T('\r'))
				estBytes--;
	}
#endif
	if (szText) *szText = szBuffer;
	if (numChars) *numChars = chars;
	return estBytes * mul + (inclBOM ? bom : 0);;
}

DWORD GetTextChars(LPCTSTR szText, BOOL unix){
	DWORD chars = 0;
#ifdef USE_RICH_EDIT
	GETTEXTLENGTHEX gtl = {0};
	if (!unix) gtl.flags = GTL_USECRLF;
	unix = FALSE;
	if (!szText) return SendMessage(client, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
#else
	if (!szText) return GetWindowTextLength(client);
#endif
	else return lstrlen(szText);
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
	CHARRANGE cr;

#ifdef USE_RICH_EDIT
	SendMessage(client, EM_EXGETSEL, 0, (LPARAM)&cr);
#else
	SendMessage(client, EM_GETSEL, (WPARAM)&cr.cpMin, (LPARAM)&cr.cpMax);
#endif
	f1 = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, FALSE, pbFindSpec);
	if (f1.cpMin < 0) {
		f2 = DoSearch(szText, cr.cpMin, cr.cpMax, bDown, bWholeWord, bCase, TRUE, pbFindSpec);
		if (f2.cpMin >= 0) {
			if (!options.bFindAutoWrap && MessageBox(hdlgFind ? hdlgFind : client, bDown ? GetString(IDS_QUERY_SEARCH_TOP) : GetString(IDS_QUERY_SEARCH_BOTTOM), STR_METAPAD, MB_OKCANCEL|MB_ICONQUESTION) == IDCANCEL) {
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
		UpdateStatus(FALSE);
		return TRUE;
	}
	MessageBox(hdlgFind ? hdlgFind : client, GetString(IDS_ERROR_SEARCH), STR_METAPAD, MB_OK|MB_ICONINFORMATION);
	return FALSE;
}





DWORD StrReplace(LPCTSTR szIn, LPTSTR* szOut, DWORD* bufLen, LPCTSTR szFind, LPCTSTR szRepl, LPBYTE pbFindSpec, LPBYTE pbReplSpec, BOOL bCase, BOOL bWholeWord, DWORD maxMatch, DWORD maxLen, BOOL matchLen){
	//0123456
	// _?*-+$
	DWORD k, m = 1, len, alen, ilen, lf, lr, ct = 0, cf = 0, cg = 0, lg = 0, nglob[6] = {0}, cglob[6] = {0}, sglob[6], gu = 0;
	WORD enc = (nFormat >> 31 ? ID_ENC_CUSTOM : (WORD)nFormat);
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
		for (k = 0; k < lf; ) if (pbFindSpec[k] < 6) nglob[pbFindSpec[k++]]++;
		for (k = 0; k < lr; ) if (pbReplSpec[k] < 6) cglob[pbReplSpec[k++]]++;
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
								if (enc == ID_ENC_UTF16 || enc == ID_ENC_UTF16BE)
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
			if (owner) MessageBox(owner, GetString(IDS_NO_SELECTED_TEXT), STR_METAPAD, MB_OK|MB_ICONINFORMATION);
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
#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
		}
		szBuf[l - lf] = _T('\0');
		SendMessage(client, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)(szBuf+lh));
		if (selection) {
			cr.cpMax = cr.cpMin + l - lf;
#ifdef USE_RICH_EDIT
			SendMessage(client, EM_EXSETSEL, 0, (LPARAM)&cr);
#else
			SendMessage(client, EM_SETSEL, (WPARAM)cr.cpMin, (LPARAM)cr.cpMax);
#endif
		}
	}
	FREE(szTmp);
	FREE(szBuf);
	SendMessage(client, WM_SETREDRAW, (WPARAM)TRUE, 0);
	InvalidateRect(client, NULL, TRUE);
	SetCursor(hCur);
	if (owner && szMsgBuf) {
		wsprintf(szMsgBuf, GetString(recur ? IDS_ITEMS_REPLACED_ITER : IDS_ITEMS_REPLACED), r, ict-1);
		MessageBox(owner, szMsgBuf, STR_METAPAD, MB_OK|MB_ICONINFORMATION);
	}
	return r;
}

