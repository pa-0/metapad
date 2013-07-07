/****************************************************************************/
/*                                                                          */
/*   metapad 3.6                                                            */
/*                                                                          */
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
#ifdef USE_RICH_EDIT
#include <richedit.h>
#endif

#include "include/outmacros.h"
#include "include/consts.h"
#include "include/strings.h"
#include "include/resource.h"
#include "include/typedefs.h"
#include "include/tmp_protos.h"

extern HANDLE globalHeap;
extern HWND hwnd;
extern HWND client;
extern BOOL bDirtyFile;
extern BOOL bLoading;
extern BOOL bReadOnly;
extern BOOL bUnix;
extern TCHAR szDir[MAXFN];
extern TCHAR szFile[MAXFN];
extern TCHAR szCaptionFile[MAXFN];
extern TCHAR szCustomFilter[2*MAXSTRING];
extern int nEncodingType;

extern option_struct options;

#ifndef USE_RICH_EDIT
static __inline void ConvertToUnix(LPTSTR szBuffer);
#else
static __inline void RichModeToDos(LPTSTR *szBuffer);
#endif

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
 * Save the current file in a file to be defined by the user.
 *
 * @return TRUE if successfully saved, FALSE if unable to save or cancelled.
 */
BOOL SaveCurrentFileAs(void)
{
	OPENFILENAME ofn;
	TCHAR szTmp[MAXFN];
	TCHAR* pch;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = client;

	if (options.bNoAutoSaveExt) {
		ofn.lpstrFilter = GetString(IDS_DEFAULT_FILTER_TEXT);
		FixFilterString((LPTSTR)ofn.lpstrFilter);
		ofn.lpstrDefExt = NULL;
	}
	else {
		ofn.lpstrFilter = szCustomFilter;
		ofn.lpstrDefExt = _T("txt");
	}

	ofn.lpstrCustomFilter = (LPTSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;

	pch = _tcsrchr(szFile, _T('\\'));
	if (pch == NULL)
		lstrcpy(szTmp, szFile);
	else
		lstrcpy(szTmp, pch+1);

	ofn.lpstrFile = szTmp;
	ofn.nMaxFile = sizeof(szTmp);

	ofn.lpstrFileTitle = (LPTSTR)NULL;
	ofn.nMaxFileTitle = 0L;
	ofn.lpstrInitialDir = szDir;
	ofn.lpstrTitle = (LPTSTR)NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;

	if (GetSaveFileName(&ofn)) {
		lstrcpy(szFile, szTmp);
		if (!SaveFile(szFile))
			return FALSE;
		SaveMRUInfo(szFile);

		SwitchReadOnly(FALSE);
		bLoading = FALSE;
		bDirtyFile = FALSE;
		UpdateStatus();
		UpdateCaption();
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
	SetCurrentDirectory(szDir);

	if (lstrlen(szFile) > 0) {
		TCHAR szTmp[MAXFN];
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
		ExpandFilename(szFile);
		wsprintf(szTmp, STR_CAPTION_FILE, szCaptionFile);
		SetWindowText(hwnd, szTmp);
		bDirtyFile = FALSE;
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
		TCHAR szBuffer[MAXFN];
		if (lstrlen(szFile) == 0) {
			if (GetWindowTextLength(client) == 0) {
				return TRUE;
			}
		}
		wsprintf(szBuffer, GetString(IDS_DIRTYFILE), szCaptionFile);
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

/**
 * Save a file.
 *
 * @param[in] szFilename A string containing the target file's name.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL SaveFile(LPCTSTR szFilename)
{
	HANDLE hFile;
	LONG lFileSize;
	DWORD dwActualBytesWritten = 0;
	LPTSTR szBuffer;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	LPTSTR pNewBuffer = NULL;

	lFileSize = GetWindowTextLength(client) * sizeof(TCHAR);
	szBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, lFileSize + (1 * sizeof(TCHAR)));

	hFile = (HANDLE)CreateFile(szFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ReportLastError();
		return FALSE;
	}

#ifdef USE_RICH_EDIT
		{
			TEXTRANGE tr;

			tr.chrg.cpMin = 0;
			tr.chrg.cpMax = -1;
			tr.lpstrText = szBuffer;

			SendMessage(client, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
		}
#else
	GetWindowText(client, szBuffer, lFileSize+1);
#endif

	if (nEncodingType == TYPE_UTF_16 || nEncodingType == TYPE_UTF_16_BE) {

		long nBytesNeeded = 0;

		if (lFileSize) {

#ifdef USE_RICH_EDIT
			RichModeToDos(&szBuffer);
#endif

#ifdef BUILD_METAPAD_UNICODE
			pNewBuffer = szBuffer;
#else
			nBytesNeeded = 2 * (1+MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szBuffer,
				lFileSize, NULL, 0));

			pNewBuffer = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, nBytesNeeded+1);
			if (pNewBuffer == NULL)
				ReportLastError();

			else if (!MultiByteToWideChar(CP_ACP, 0,	(LPCSTR)szBuffer, lFileSize,
				(LPWSTR)pNewBuffer, nBytesNeeded)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_STRING_ERROR));
			}
#endif
		}

		if ((nEncodingType == TYPE_UTF_16_BE) && pNewBuffer) {
			const CHAR szBOM_UTF_16_BE[SIZEOFBOM_UTF_16] = {'\376', '\377'};
			ReverseBytes((PBYTE)pNewBuffer, nBytesNeeded);
			// 0xFE, 0xFF - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_16_BE, SIZEOFBOM_UTF_16, &dwActualBytesWritten, NULL);
		}
		else if (nEncodingType == TYPE_UTF_16) {
			const CHAR szBOM_UTF_16[SIZEOFBOM_UTF_16] = {'\377', '\376'};
			// 0xFF, 0xFE - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_16, SIZEOFBOM_UTF_16, &dwActualBytesWritten, NULL);
		}

		if (dwActualBytesWritten != SIZEOFBOM_UTF_16) {
			ERROROUT(GetString(IDS_UNICODE_BOM_ERROR));
		}

		if (lFileSize) {
			if (!WriteFile(hFile, pNewBuffer, nBytesNeeded-2, &dwActualBytesWritten, NULL)) {
				ReportLastError();
			}
			lFileSize = nBytesNeeded-2;
#ifndef BUILD_METAPAD_UNICODE
			HeapFree(globalHeap, 0, (HGLOBAL)pNewBuffer);
#endif
		}
		else {
			dwActualBytesWritten = 0;
		}
	}
	else {

#ifdef USE_RICH_EDIT
		if (bUnix) {
			int i = 0;
			while (szBuffer[i] != _T('\0')) {
				if (szBuffer[i] == _T('\r'))
					szBuffer[i] = _T('\n');
				i++;
			}
			lFileSize = lstrlen(szBuffer);
		}
		else {
			RichModeToDos(&szBuffer);
		}
#else
		if (bUnix) {
			ConvertToUnix(szBuffer);
			lFileSize = lstrlen(szBuffer);
		}
#endif
		if (nEncodingType == TYPE_UTF_8) {
			const CHAR szBOM_UTF_8[SIZEOFBOM_UTF_8] = {'\xEF', '\xBB', '\xBF'};
			// 0xEF, 0xBB, 0xBF / "\357\273\277" - leave off _T() macro.
			WriteFile(hFile, szBOM_UTF_8, SIZEOFBOM_UTF_8, &dwActualBytesWritten, NULL);

			if (dwActualBytesWritten != SIZEOFBOM_UTF_8) {
				ERROROUT(GetString(IDS_UNICODE_BOM_ERROR));
			}
		}

		if (!WriteFile(hFile, szBuffer, lFileSize, &dwActualBytesWritten, NULL)) {
			ReportLastError();
		}
	}

	SetEndOfFile(hFile);
	CloseHandle(hFile);

	SetCursor(hcur);
	HeapFree(globalHeap, 0, (HGLOBAL)szBuffer);
	if (dwActualBytesWritten != (DWORD)lFileSize) {
		ERROROUT(GetString(IDS_ERROR_LOCKED));
		return FALSE;
	}
	else
		return TRUE;
}

#ifndef USE_RICH_EDIT
/**
 * Convert a string from DOS mode to Unix mode.
 *
 * @param[in] szBuffer String to convert.
 * @note This conversion consist in removing carriage returns.
 */
void ConvertToUnix(LPTSTR szBuffer)
{
	UINT i = 0, j = 0;
	LPTSTR szTemp = (LPTSTR) HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lstrlen(szBuffer)+1) * sizeof(TCHAR));

	lstrcpy(szTemp, szBuffer);

	while (szTemp[i] != _T('\0')) {
		if (szTemp[i] != _T('\r')) {
			szBuffer[j] = szTemp[i];
			j++;
		}
		i++;
	}
	szBuffer[j] = _T('\0');
	HeapFree(globalHeap, 0, (HGLOBAL)szTemp);
}
#else
/**
 * Convert a string from rich text mode to DOS mode.
 *
 * @param[in] szBuffer Pointer to the string to convert.
 * @note This conversion consists in adding one line break after every carriage return.
 */
void RichModeToDos(LPTSTR *szBuffer)
{
	int cnt = 0;
	int i = 0, j;

	while ((*szBuffer)[i] != _T('\0')) {
		if ((*szBuffer)[i] == _T('\r'))
			cnt++;
		i++;
	}

	if (cnt) {
		LPTSTR szNewBuffer = (LPTSTR)HeapAlloc(globalHeap, HEAP_ZERO_MEMORY, (lstrlen(*szBuffer)+cnt+2) * sizeof(TCHAR));
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

		HeapFree(globalHeap, 0, (HGLOBAL) *szBuffer);
		*szBuffer = szNewBuffer;
	}
}
#endif
