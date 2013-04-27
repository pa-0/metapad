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

#include <windows.h>
#include <tchar.h>

#include "include/typedefs.h"
#include "include/tmp_protos.h"
#include "include/external_viewers.h"

#include "include/strings.h"
#include "include/outmacros.h"
#include "include/resource.h"

extern HWND hwnd;
extern TCHAR szCaptionFile[];
extern TCHAR szFile[];
extern TCHAR szDir[];
extern BOOL bDirtyFile;

extern option_struct options;


BOOL ExecuteProgram(LPCTSTR lpExecutable, LPCTSTR lpCommandLine)
{
	TCHAR szCmdLine[1024];
	LPTSTR lpFormat;

	if (lpExecutable[0] == _T('"') && lpExecutable[lstrlen(lpExecutable) - 1] == _T('"')) {
		// quotes already present
		lpFormat = _T("%s %s");
	}
	else {
		// executable file must be quoted to conform to Win32 file name
		// specs.
		lpFormat = _T("\"%s\" %s");
	}

	wsprintf(szCmdLine, lpFormat, lpExecutable, lpCommandLine);

	if (lstrcmpi(lpExecutable + (lstrlen(lpExecutable) - 4), ".exe") != 0) {
		if ((int)ShellExecute(NULL, NULL, lpExecutable, szCmdLine, szDir, SW_SHOWNORMAL) <= 32) {
			return FALSE;
		}
	}
	else {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(STARTUPINFO));

		si.cb = sizeof(STARTUPINFO);
		si.wShowWindow = SW_SHOWNORMAL;
		si.dwFlags = STARTF_USESHOWWINDOW;

		if (!CreateProcess(lpExecutable, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			return FALSE;
		}
		else {
			// We don't use the handles so close them now
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	return TRUE;
}

void LaunchPrimaryExternalViewer(void)
{
	TCHAR szLaunch[MAXFN] = {'\0'};

	lstrcat(szLaunch, options.szArgs);
	lstrcat(szLaunch, _T(" \""));
	lstrcat(szLaunch, szFile);
	lstrcat(szLaunch, _T("\""));
	if (!ExecuteProgram(options.szBrowser, szLaunch))
		ERROROUT(GetString(IDS_PRIMARY_VIEWER_ERROR));
}

void LaunchSecondaryExternalViewer(void)
{
	TCHAR szLaunch[MAXFN] = {'\0'};

	lstrcat(szLaunch, options.szArgs2);
	lstrcat(szLaunch, _T(" \""));
	lstrcat(szLaunch, szFile);
	lstrcat(szLaunch, _T("\""));
	if (!ExecuteProgram(options.szBrowser2, szLaunch))
		ERROROUT(GetString(IDS_SECONDARY_VIEWER_ERROR));
}

void LaunchInViewer(BOOL bCustom, BOOL bSecondary)
{
	if (bCustom) {
		if (!bSecondary && options.szBrowser[0] == '\0') {
			MessageBox(hwnd, GetString(IDS_PRIMARY_VIEWER_MISSING), STR_METAPAD, MB_OK|MB_ICONEXCLAMATION);
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
			return;
		}
//DBGOUT(options.szBrowser2, "run szBrowser2 value:");
		if (bSecondary && options.szBrowser2[0] == '\0') {
			MessageBox(hwnd, GetString(IDS_SECONDARY_VIEWER_MISSING), STR_METAPAD, MB_OK|MB_ICONEXCLAMATION);
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_VIEW_OPTIONS, 0), 0);
			return;
		}
	}

	if (szFile[0] == '\0' || (bDirtyFile && options.nLaunchSave != 2)) {
		int res = IDYES;
		if (options.nLaunchSave == 0) {
			TCHAR szBuffer[MAXFN];
			wsprintf(szBuffer, GetString(IDS_DIRTYFILE), szCaptionFile);
			res = MessageBox(hwnd, szBuffer, STR_METAPAD, MB_ICONEXCLAMATION | MB_YESNOCANCEL);
		}
		if (res == IDCANCEL) {
			return;
		}
		else if (res == IDYES) {
			if (!SaveCurrentFile()) {
				return;
			}
		}
	}
	if (szFile[0] != '\0') {
		if (bCustom) {
			if (bSecondary) {
				LaunchSecondaryExternalViewer();
			}
			else {
				LaunchPrimaryExternalViewer();
			}
		}
		else {
			int ret = (int)ShellExecute(NULL, _T("open"), szFile, NULL, szDir, SW_SHOW);
			if (ret <= 32) {
				switch (ret) {
				case SE_ERR_NOASSOC:
					ERROROUT(GetString(IDS_NO_DEFAULT_VIEWER));
					break;
				default:
					ERROROUT(GetString(IDS_DEFAULT_VIEWER_ERROR));
				}
			}
		}
	}

	if (options.bLaunchClose) {
		DestroyWindow(hwnd);
	}
}
