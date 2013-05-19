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

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#include "include/typedefs.h"
#include "include/resource.h"
#include "include/outmacros.h"
#include "include/strings.h"
#include "include/language_plugin.h"
#include "include/tmp_protos.h"

extern HWND hwnd;
extern BOOL g_bDisablePluginVersionChecking;
extern HINSTANCE hinstThis;
extern HINSTANCE hinstLang;

extern option_struct options;

/**
 * Loads and verify a language plugin.
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
 * Finds and loads a language plugin, which path is stored in options.szLangPlugin.
 * 
 * Finds and loads a language plugin, which path is stored in options.szLangPlugin.
 * Defaults to english if unable to load a plugin.
 */
void FindAndLoadLanguagePlugin(void)
{
	HINSTANCE hinstTemp;

	hinstLang = hinstThis;

	if (options.szLangPlugin[0] == '\0')
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
