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

#ifdef UNICODE
#include <wchar.h>
#endif

#undef _tWinMain
#ifdef _UNICODE
#define _tWinMain wWinMain
#else
#define _tWinMain WinMain
#endif

void __cdecl _tWinMainCRTStartup(void)
{
	int mainret;
	LPTSTR lpszCommandLine;
	STARTUPINFO StartupInfo;

	lpszCommandLine = (LPTSTR)GetCommandLine();

	while ( *lpszCommandLine && (*lpszCommandLine <= _T(' ') ) )
        lpszCommandLine++;

	if (*lpszCommandLine == _T('"') ) {
		lpszCommandLine++;
        while(*lpszCommandLine && (*lpszCommandLine != _T('"') ) )
            lpszCommandLine++;

        if (*lpszCommandLine == _T('"') )
            lpszCommandLine++;
	} else {
		while (*lpszCommandLine > _T(' ') )
			lpszCommandLine++;
	}

	while ( *lpszCommandLine && (*lpszCommandLine <= _T(' ') ) )
        lpszCommandLine++;

	StartupInfo.dwFlags = 0;
	GetStartupInfo(&StartupInfo);

	mainret = _tWinMain( GetModuleHandle(NULL),
				NULL,
				lpszCommandLine,
				StartupInfo.dwFlags &
				STARTF_USESHOWWINDOW ?
				StartupInfo.wShowWindow : SW_SHOWDEFAULT );

	ExitProcess(mainret);
}

#ifdef _DEBUG
int main(){
	_tWinMainCRTStartup();
}
#endif