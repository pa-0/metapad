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
 * @file file_save.c
 * @brief File saving functions.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <commdlg.h>
#include <tchar.h>

#include "include/consts.h"
#include "include/globals.h"
#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"
#include "include/metapad.h"
#include "include/encoding.h"


BOOL SaveFile(LPCTSTR szFilename);


/**
 * Replace '|' with '\0', and adds a '\0' at the end.
 *
 * @param[in] szIn String to fix.
 */
void FixFilterString(LPTSTR szIn)
{
	int i;

	for (i = 0; szIn[i]; ++i) {
		if (szIn[i] == _T('|')) {
			szIn[i] = _T('\0');
		}
	}
	szIn[i+1] = _T('\0');
}

/**
 * Save a file.
 *
 * @param[in] szFilename A string containing the target file's name.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL SaveFile(LPCTSTR szFilename)
{
	HANDLE hFile;
	DWORD i, dwActualBytesWritten = 0, nChars, nBytes = 0;
	LPCTSTR szBuffer;
	LPTSTR szTmp = NULL;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	UINT nonansi = 0, cp = CP_ACP;
	TCHAR szUncFn[MAXFN+6] = _T("\\\\?\\");
	BOOL bufDirty = FALSE;

/*	szBuffer = GetShadowBuffer(&nChars);
	lstrcpy(szUncFn+4, szFilename);
	hFile = (HANDLE)CreateFile(szUncFn, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		SetCursor(hcur);
		ReportLastError();
		return FALSE;
	}

#ifdef UNICODE
	if (bBinaryFile && sizeof(TCHAR) > 1) {
		szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, (nChars+1) * sizeof(TCHAR));
		lstrcpy(szTmp, szBuffer);
		for (i = 0; i < nChars; i++)
			if (szTmp[i] == _T('\x2400'))
				szTmp[i] = _T('\0');
		szBuffer = (LPCTSTR)szTmp;
		bufDirty = TRUE;
	}
#endif

	if (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE) {
		if (nChars) {
#ifdef USE_RICH_EDIT
			RichModeToDos(&szBuffer, &bufDirty);
#endif
#ifndef UNICODE
			if (sizeof(TCHAR) < 2) {
				nBytes = 2 * MultiByteToWideChar(cp, 0, (LPCSTR)szBuffer, nChars, NULL, 0);
				szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, nBytes+1);
				if (!szTmp)
					ReportLastError();
				else if (!MultiByteToWideChar(cp, 0, (LPCSTR)szBuffer, nChars, (LPWSTR)szTmp, nBytes)) {
					ReportLastError();
					ERROROUT(GetString(IDS_UNICODE_STRING_ERROR));
				}
				if (bufDirty) FREE(szBuffer);
				szBuffer = (LPCTSTR)szTmp;
				bufDirty = TRUE;
			} else
#endif
				nBytes = nChars * sizeof(TCHAR);
		}

		if (nEncodingType == TYPE_UTF_16_BE) {
			const CHAR szBOM_UTF_16_BE[SIZEOFBOM_UTF_16] = {'\376', '\377'};
			if (!bufDirty) {
				szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, nBytes + sizeof(TCHAR));
				lstrcpy(szTmp, szBuffer);
				szBuffer = (LPCTSTR)szTmp;
				bufDirty = TRUE;
			}
			ReverseBytes((LPBYTE)szTmp, nBytes);
			// 0xFE, 0xFF - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_16_BE, SIZEOFBOM_UTF_16, &dwActualBytesWritten, NULL);
		} else {
			const CHAR szBOM_UTF_16[SIZEOFBOM_UTF_16] = {'\377', '\376'};
			// 0xFF, 0xFE - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_16, SIZEOFBOM_UTF_16, &dwActualBytesWritten, NULL);
		}

		if (dwActualBytesWritten != SIZEOFBOM_UTF_16)
			ERROROUT(GetString(IDS_UNICODE_BOM_ERROR));

		if (nChars) {
			if (!WriteFile(hFile, szBuffer, nBytes, &dwActualBytesWritten, NULL))
				ReportLastError();
		}
		else
			dwActualBytesWritten = 0;
	} else {
		if (bUnix) {
			if (!bufDirty) {
				szTmp = (LPTSTR)HeapAlloc(globalHeap, 0, (nChars+1) * sizeof(TCHAR));
				lstrcpy(szTmp, szBuffer);
				szBuffer = (LPCTSTR)szTmp;
				bufDirty = TRUE;
			}
#ifdef USE_RICH_EDIT
			for (i = 0; szTmp[i]; i++) {
				if (szTmp[i] == _T('\r'))
					szTmp[i] = _T('\n');
			}
#else
			ConvertToUnix(szTmp, &nChars);
#endif
		}
#ifdef USE_RICH_EDIT
		else
			RichModeToDos(&szBuffer, &bufDirty);
#endif

		if (nEncodingType == TYPE_UTF_8) {
			const CHAR szBOM_UTF_8[SIZEOFBOM_UTF_8] = {'\xEF', '\xBB', '\xBF'};
			// 0xEF, 0xBB, 0xBF / "\357\273\277" - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_8, SIZEOFBOM_UTF_8, &dwActualBytesWritten, NULL);

			if (dwActualBytesWritten != SIZEOFBOM_UTF_8)
				ERROROUT(GetString(IDS_UNICODE_BOM_ERROR));
			cp = CP_UTF8;
		}

		nBytes = nChars;
#ifdef UNICODE
		if (nChars && sizeof(TCHAR) > 1) {	//if we're internally Unicode and the buffer is non-empty, conversion is needed
			nBytes = WideCharToMultiByte(cp, 0, szBuffer, nChars, NULL, 0, NULL, NULL);
			if (!(szTmp = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nBytes)))
				ReportLastError();
			else if (!WideCharToMultiByte(cp, 0, szBuffer, nChars, (LPSTR)szTmp, nBytes, NULL, (cp != CP_ACP ? NULL : &nonansi)))
				ReportLastError();
			else if (nonansi)
				ERROROUT(GetString(IDS_UNICODE_SAVE_TRUNCATION));
			if (bufDirty) FREE(szBuffer);
			szBuffer = (LPCTSTR)szTmp;
			bufDirty = TRUE;
		}
#endif
		if (!WriteFile(hFile, szBuffer, nBytes, &dwActualBytesWritten, NULL))
			ReportLastError();
	}

	SetEndOfFile(hFile);
	CloseHandle(hFile);
	if (bufDirty) FREE(szBuffer);
	SetCursor(hcur);
	if (dwActualBytesWritten != (DWORD)nBytes) {
		ERROROUT(GetString(IDS_ERROR_LOCKED));
		return FALSE;
	}*/
	return TRUE;
}

/**
 * Save the current file in a file to be defined by the user.
 *
 * @return TRUE if successfully saved, FALSE if unable to save or cancelled.
 */
BOOL SaveCurrentFileAs(void)
{
	OPENFILENAME ofn;
	TCHAR szTmp[MAXFN] = _T("");
	TCHAR* pch;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = client;

	if (options.bNoAutoSaveExt) {
		ofn.lpstrFilter = GetString(IDS_DEFAULT_FILTER_TEXT);
		FixFilterString((LPTSTR)ofn.lpstrFilter);
		ofn.lpstrDefExt = NULL;
	} else {
		ofn.lpstrFilter = SCNUL(szCustomFilter);
		ofn.lpstrDefExt = _T("txt");
	}

	ofn.lpstrCustomFilter = (LPTSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;

	if (szFile){
		pch = _tcsrchr(szFile, _T('\\'));
		if (pch == NULL)
			lstrcpy(szTmp, szFile);
		else
			lstrcpy(szTmp, pch+1);
	}

	ofn.lpstrFile = szTmp;
	ofn.nMaxFile = MAXFN;

	ofn.lpstrFileTitle = (LPTSTR)NULL;
	ofn.nMaxFileTitle = 0L;
	ofn.lpstrInitialDir = SCNUL(szDir);
	ofn.lpstrTitle = (LPTSTR)NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;

	if (GetSaveFileName(&ofn)) {
		SSTRCPY(szFile, szTmp);
		if (!SaveFile(szFile))
			return FALSE;
		SaveMRUInfo(szFile);

		SwitchReadOnly(FALSE);
		bLoading = FALSE;
		bDirtyFile = FALSE;
		UpdateStatus(TRUE);
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * Save current file.
 *
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL SaveCurrentFile(void)
{
	SetCurrentDirectory(SCNUL(szDir));

	if (SCNUL(szFile)[0]) {
		DWORD dwResult = GetFileAttributes(szFile);

		if (dwResult != 0xffffffff && bReadOnly != (BOOL)(dwResult & FILE_ATTRIBUTE_READONLY)) {
			bReadOnly = dwResult & FILE_ATTRIBUTE_READONLY;
			UpdateCaption();
		}
		if (bReadOnly) {
			if (MessageBox(hwnd, GetString(IDS_READONLY_WARNING), STR_METAPAD, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK) {
				return SaveCurrentFileAs();
			}
			return FALSE;
		}

		if (!SaveFile(szFile))
			return FALSE;
		ExpandFilename(szFile, &szFile);
		bDirtyFile = FALSE;
		UpdateCaption();
		return TRUE;
	}
	else
		return SaveCurrentFileAs();
}


/**
 * If the current file has been modified, prompt a message box asking the user
 * if saving the changes is wanted.
 *
 * @return If the file hasn't been modified, TRUE. If the file has been modified
 * and the save is successful or unwanted, TRUE.
 * If the save is unsuccessful or the user chooses CANCEL, return FALSE.
 */
BOOL SaveIfDirty(void)
{
	if (bDirtyFile) {
		TCHAR szBuffer[MAXFN+MAXSTRING];
		if (!SCNUL(szFile)[0]) {
			if (!GetWindowTextLength(client))
				return TRUE;
		}
		wsprintf(szBuffer, GetString(IDS_DIRTYFILE), SCNUL8(szCaptionFile)+8);
		switch (MessageBox(hwnd, szBuffer, STR_METAPAD, MB_ICONEXCLAMATION | MB_YESNOCANCEL)) {
			case IDYES:
				if (!SaveCurrentFile())
					return FALSE;
			case IDNO:
				return TRUE;
			case IDCANCEL:
				return FALSE;
		}
	}
	return TRUE;
}



#ifndef USE_RICH_EDIT
/**
 * Convert a string from DOS mode to Unix mode.
 *
 * @param[in] szBuffer String to convert.
 * @note This conversion consist in removing carriage returns.
 */
static __inline void ConvertToUnix(LPTSTR szBuffer, DWORD* len)
{
	LPTSTR szPtr = szBuffer;
	for (; *szBuffer; szBuffer++) {
		if (*szBuffer != _T('\r'))
			*szPtr++ = *szBuffer;
	}
	*szPtr = _T('\0');
	if (len) *len -= szBuffer - szPtr;
}
#else
/**
 * Convert a string from rich text mode to DOS mode.
 *
 * @param[in] szBuffer Pointer to the string to convert.
 * @note This conversion consists in adding one line break after every carriage return.
 */
static __inline void RichModeToDos(LPCTSTR *szBuffer, BOOL *bufDirty)
{
	int cnt = 0;
	int i = 0, j;

	while ((*szBuffer)[i] != _T('\0')) {
		if ((*szBuffer)[i] == _T('\r'))
			cnt++;
		i++;
	}

	if (cnt) {
		LPTSTR szNewBuffer = (LPTSTR)HeapAlloc(globalHeap, 0, (lstrlen(*szBuffer)+cnt+2) * sizeof(TCHAR));
		i = j = 0;
		while ((*szBuffer)[i] != _T('\0')) {
			if ((*szBuffer)[i] == _T('\r')) {
				szNewBuffer[j++] = (*szBuffer)[i];
				szNewBuffer[j] = _T('\n');
			}
			else {
				szNewBuffer[j] = (*szBuffer)[i];
			}
			j++; i++;
		}
		szNewBuffer[j] = _T('\0');
		if (bufDirty){
			if (*bufDirty) FREE(*szBuffer);
			*bufDirty = TRUE;
		}
		*szBuffer = szNewBuffer;
	}
}
#endif

