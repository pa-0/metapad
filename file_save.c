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



/**
 * Save a file.
 *
 * @param[in] szFilename A string containing the target file's name.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL SaveFile(LPCTSTR szFilename, BOOL bMRU) {
	HANDLE hFile;
	DWORD written = 0, nChars, nBytes;
	LPTSTR szBuffer, szEncd = NULL;
	LPBYTE bom;
	HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
	UINT nonansi = 0, cp = CP_ACP;
	BOOL bufDirty = FALSE, fail = FALSE;
	WORD enc = (nFormat >> 31) ? ID_ENC_CUSTOM : (WORD)nFormat;
	WORD lfmt = (nFormat >> 16) & 0xfff;
	DWORD lines = SendMessage(client, EM_GETLINECOUNT, 0, 0);

	szBuffer = (LPTSTR)GetShadowBuffer(&nChars);
	nBytes = nChars;
	if (nChars) {
		ExportLineFmt(&szBuffer, &nChars, lfmt, lines, &bufDirty);
		if (enc == ID_ENC_BIN)
			ExportBinary(szBuffer, nChars);
		for ( ; 1; fail = FALSE) {
			if (szEncd != szBuffer) FREE(szEncd);
			szEncd = szBuffer;
			nBytes = EncodeText((LPBYTE*)&szEncd, nChars, nFormat, NULL, &fail);
			if (enc != ID_ENC_UTF8 && fail) {
				switch (MessageBox(hwnd, GetString(IDS_UNICODE_SAVE_TRUNCATION), STR_METAPAD, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
					case IDYES:
						SetFileFormat(enc = ID_ENC_UTF8, 0);
						continue;
					case IDNO:
						fail = FALSE;
				}
			}
			break;
		}
		if (szEncd != szBuffer) {
			if (bufDirty) FREE(szBuffer);
			bufDirty = TRUE;
			szBuffer = szEncd;
		}
	}
	if (!fail) {
		hFile = (HANDLE)CreateFile(szFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			SetCursor(hcur);
			ReportLastError();
			return FALSE;
		}
		nChars = GetBOM(&bom, enc);
		if (nChars) {
			WriteFile(hFile, bom, nChars, &written, NULL);
			if (written != nChars)
				ERROROUT(GetString(IDS_WRITE_BOM_ERROR));
		}
		if (!WriteFile(hFile, szBuffer, nBytes, &written, NULL))
			ReportLastError();
		SetEndOfFile(hFile);
		CloseHandle(hFile);
		if (written != nBytes) {
			ERROROUT(GetString(IDS_ERROR_LOCKED));
			fail = TRUE;
		}
	}
	if (bufDirty) FREE(szBuffer);
	if (!fail){
		if (szFile != szFilename){
			SSTRCPY(szFile, szFilename);
			SSTRCPY(szDir, szFilename);
			szBuffer = lstrrchr(szDir, _T('\\'));
			if (szBuffer) *szBuffer = _T('\0');
		}
		FREE(szCaptionFile);
		if (bMRU) SaveMRUInfo(szFile);
		SwitchReadOnly(FALSE);
		bLoading = FALSE;
		UpdateSavedInfo();
	}
	UpdateStatus(TRUE);
	SetCursor(hcur);
	return !fail;
}


/**
 * Save the current file in a file to be defined by the user.
 *
 * @return TRUE if successfully saved, FALSE if unable to save or cancelled.
 */
BOOL SaveCurrentFileAs(void) {
	OPENFILENAME ofn;
	TCHAR fn[MAXFN] = _T("");
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
		pch = lstrrchr(szFile, _T('\\'));
		if (pch == NULL) 	lstrcpy(fn, szFile);
		else 				lstrcpy(fn, pch+1);
	}
	ofn.lpstrFile = fn;
	ofn.nMaxFile = MAXFN;
	ofn.lpstrFileTitle = (LPTSTR)NULL;
	ofn.nMaxFileTitle = 0L;
	ofn.lpstrInitialDir = SCNUL(szDir);
	ofn.lpstrTitle = (LPTSTR)NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	if (!GetSaveFileName(&ofn)) return FALSE;
	FixShortFilename(fn, (LPTSTR*)&fn);
	return SaveFile(fn, TRUE);
}

/**
 * Save current file.
 *
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL SaveCurrentFile(void) {
	//SetCurrentDirectory(SCNUL(szDir));
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
		return SaveFile(szFile, FALSE);
	} else
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
BOOL SaveIfDirty(void) {
	TCHAR szBuffer[MAXFN+MAXSTRING];
	if (bDirtyFile) {
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
