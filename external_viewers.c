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

/**
 * @file external_viewers.c
 * @brief External viewers functions.
 */

#include "include/metapad.h"
#include <shellapi.h>

/**
 * Try to execute a third party program.
 *
 * @param lpExecutable Path to program executable.
 * @param lpCommandLine Command line arguments to pass to program.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL ExecuteProgram(LPCTSTR lpExecutable, LPCTSTR lpCommandLine) {
	LPTSTR lpFormat;
	LPTSTR szCmdLine = kallocsz(lstrlen(lpExecutable)+lstrlen(lpCommandLine)+5);
	if (lpExecutable[0] == _T('"') && lpExecutable[lstrlen(lpExecutable) - 1] == _T('"'))
		lpFormat = _T("%s %s");			// quotes already present
	else
		lpFormat = _T("\"%s\" %s");		// executable file must be quoted to conform to Win32 file name specs.
	wsprintf(szCmdLine, lpFormat, lpExecutable, lpCommandLine);

	if (lstrcmpi(lpExecutable + (lstrlen(lpExecutable) - 4), _T(".exe")) != 0) {
		/// @todo Should this inform about which error happened?
		if ((INT_PTR)ShellExecute(NULL, NULL, lpExecutable, lpCommandLine, szDir, SW_SHOWNORMAL) <= 32) {
			return FALSE;
		}
	} else {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(STARTUPINFO));

		si.cb = sizeof(STARTUPINFO);
		si.wShowWindow = SW_SHOWNORMAL;
		si.dwFlags = STARTF_USESHOWWINDOW;
		if (!CreateProcess(lpExecutable, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			return FALSE;
		else {	// We don't use the handles so close them now
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	kfree(&szCmdLine);
	return TRUE;
}

/**
 * Launch the primary external viewer.
 */
void LaunchExternalViewer(int id, LPCTSTR fn) {
	TCHAR szLaunch[MAXFN+MAXARGS+8] = {_T('\0')};
	LPTSTR args = id ? options.szArgs2 : options.szArgs;
	lstrcat(szLaunch, SCNUL(args));
	lstrcat(szLaunch, _T(" \""));
	lstrcat(szLaunch, SCNULD(fn, SCNUL(szFile)));
	lstrcat(szLaunch, _T("\""));
	if (!ExecuteProgram(SCNUL(id ? options.szBrowser2 : options.szBrowser), szLaunch))
		ERROROUT(GetString(IDS_VIEWER_ERROR));
}

/**
 * Open current file on an external viewer.
 *
 * @param bCustom TRUE to use one of the custom viewers, FALSE to use associated program.
 * @param bSecondary TRUE to use secondary viewer, FALSE to use primary viewer. Ignored if the bCustom is FALSE.
 */
void LaunchInViewer(BOOL bCustom, BOOL bSecondary) {
	TCHAR fnbuf[MAXFN];
	LPTSTR fn = fnbuf, prg = bSecondary ? options.szBrowser2 : options.szBrowser;
	
	if (bCustom)
		if (!SCNUL(prg)[0]) {
			MessageBox(hwnd, GetString(IDS_VIEWER_MISSING), GetString(STR_METAPAD), MB_OK|MB_ICONEXCLAMATION);
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
			return;
		}

	if (SCNUL(szFile)[0] == _T('\0') || (bDirtyFile && options.nLaunchSave != 2)) {
		int res = IDYES;
		if (options.nLaunchSave == 0) {
			TCHAR szBuffer[MAXFN + MAXSTRING];
			wsprintf(szBuffer, GetString(IDS_DIRTYFILE), SCNUL8(szCaptionFile)+8);
			res = MessageBox(hwnd, szBuffer, GetString(STR_METAPAD), MB_ICONEXCLAMATION | MB_YESNOCANCEL);
		}
		if (res == IDCANCEL)
			return;
		else if (res == IDYES) {
			if (!SaveCurrentFile())
				return;
		}
	}
	if (SCNUL(szFile)[0] != _T('\0')) {
		lstrcpy(fn, szFile);
		if (lstrlen(fn) > MAX_PATH+2) { 
			if (!(GetShortPathName(fn, fn, MAXFN))) lstrcpy(fn, szFile);
			else if (lstrlen(fn) <= MAX_PATH+2) GetReadableFilename(fn, &fn);
		} else GetReadableFilename(fn, &fn);
		if (bCustom)
			LaunchExternalViewer((int)bSecondary, fn);
		else {
			INT_PTR ret = (INT_PTR)ShellExecute(NULL, _T("open"), fn, NULL, SCNUL(szDir), SW_SHOW);
			if (ret <= 32) {
				switch (ret) {
				case SE_ERR_NOASSOC:
					ERROROUT(GetString(IDS_NO_DEFAULT_VIEWER));
					break;
				default:
					ERROROUT(GetString(IDS_VIEWER_ERROR));
				}
			}
		}
	}

	if (options.bLaunchClose)
		DestroyWindow(hwnd);
}
