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
 * @file language_plugin.c
 * @brief Language plugin loading functions.
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#include "include/typedefs.h"
#include "include/resource.h"
#include "include/macros.h"
#include "include/strings.h"
#include "include/language_plugin.h"
#include "include/tmp_protos.h"

extern HWND hwnd;
extern BOOL g_bDisablePluginVersionChecking;
extern HINSTANCE hinstThis;
extern HINSTANCE hinstLang;

extern option_struct options;

/**
 * Load and verify a language plugin.
 *
 * @param szPlugin Path to language plugin.
 * @return NULL if unable to load the plugin, an instance to the plugin otherwise.
 */
HINSTANCE LoadAndVerifyLanguagePlugin(LPCTSTR szPlugin)
{
	HINSTANCE hinstTemp;

	hinstTemp = LoadLibrary(szPlugin);
	if (hinstTemp == NULL) {
		ERROROUT(GetString(IDS_INVALID_PLUGIN_ERROR));
		return NULL;
	}

	{
		TCHAR szVersionThis[25];
		TCHAR szVersionPlug[25];

		if (LoadString(hinstTemp, IDS_VERSION_SYNCH, szVersionPlug, 25) == 0) {
			ERROROUT(GetString(IDS_BAD_STRING_PLUGIN_ERROR));
			FreeLibrary(hinstTemp);
			return NULL;
		}
		LoadString(hinstThis, IDS_VERSION_SYNCH, szVersionThis, 25);
		if (!g_bDisablePluginVersionChecking && lstrcmpi(szVersionThis, szVersionPlug) != 0) {
			TCHAR szVersionError[550];
			wsprintf(szVersionError, GetString(IDS_PLUGIN_MISMATCH_ERROR), szVersionPlug, szVersionThis);
			ERROROUT(szVersionError);
		}
	}

	return hinstTemp;
}

/**
 * Find and load a language plugin.
 *
 * @note Plugin's path is stored in options.szLangPlugin.
 * @note Default to english if unable to load a plugin.
 */
void FindAndLoadLanguagePlugin(void)
{
	HINSTANCE hinstTemp;

	hinstLang = hinstThis;

	if (options.szLangPlugin[0] == _T('\0'))
		return;

	{
		WIN32_FIND_DATA FileData;
		HANDLE hSearch;

		hSearch = FindFirstFile(options.szLangPlugin, &FileData);
		if (hSearch == INVALID_HANDLE_VALUE) {
			ERROROUT(_T("Could not find the language plugin DLL."));
			goto badplugin;
		}
		else {
			FindClose(hSearch);
		}
	}

	hinstTemp = LoadAndVerifyLanguagePlugin(options.szLangPlugin);
	if (hinstTemp) {
		hinstLang = hinstTemp;
		return;
	}

badplugin:
	ERROROUT(_T("Temporarily reverting language to Default (English)\n\nCheck the language plugin setting."));
}
