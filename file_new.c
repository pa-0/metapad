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
#include "include/file_utils.h"
#include "include/resource.h"
#include "include/tmp_protos.h"
#include "include/typedefs.h"
#include "include/strings.h"
#include "include/macros.h"

extern HANDLE globalHeap;
extern BOOL bBinaryFile;
extern BOOL bDirtyFile;
extern BOOL bLoading;
extern HWND client;
extern HWND hwnd;
extern LPTSTR lpszShadow;
extern LPTSTR szCaptionFile;
extern LPTSTR szFile;

extern option_struct options;

void MakeNewFile(void)
{
	bLoading = TRUE;
	SetFileFormat(options.nFormatIndex);
	SetWindowText(client, _T(""));
	bDirtyFile = FALSE;
	bBinaryFile = FALSE;
	bLoading = FALSE;

	{
		TCHAR szBuffer[100];
		wsprintf(szBuffer, STR_CAPTION_FILE, GetString(IDS_NEW_FILE));
		SetWindowText(hwnd, szBuffer);
	}

	SwitchReadOnly(FALSE);
	FREE(szFile);
	SSTRCPY(szCaptionFile, GetString(IDS_NEW_FILE));
	UpdateStatus();
	if (lpszShadow)
		lpszShadow[0] = _T('\0');
	bLoading = FALSE;
}
