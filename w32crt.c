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

#ifdef UNICODE
#include <wchar.h>
#endif

void __cdecl _tWinMainCRTStartup(void)
{
    int mainret;
    LPTSTR lpszCommandLine;
    STARTUPINFO StartupInfo;

    lpszCommandLine = (LPTSTR)GetCommandLine();

    if (*lpszCommandLine == _T('"') ) {
		lpszCommandLine++;
        while(*lpszCommandLine && (*lpszCommandLine != _T('"')) )
            lpszCommandLine++;

        if (*lpszCommandLine == T('"') )
            lpszCommandLine++;
    }
    else {
        while (*lpszCommandLine > _T(' ') )
            lpszCommandLine++;
    }

    while ( *lpszCommandLine && (*lpszCommandLine <= _T(' ') )
        lpszCommandLine++;

    StartupInfo.dwFlags = 0;
    GetStartupInfo(&StartupInfo);

#ifdef UNICODE
    mainret = wWinMain( GetModuleHandle(NULL),
#else
    mainret = WinMain( GetModuleHandle(NULL),
#endif
                       NULL,
                       lpszCommandLine,
                       StartupInfo.dwFlags & STARTF_USESHOWWINDOW
                       ? StartupInfo.wShowWindow : SW_SHOWDEFAULT );

    ExitProcess(mainret);
}
/*
int __cdecl _isctype(int c , int mask)
{
    if (((unsigned)(c + 1)) <= 256)
        return (_pctype[c] & mask);
    else
        return 0;
}
*/
/*
long __cdecl _ttol(const char* pstr)
{
    int  cCurr;
    long lTotal;
    int  iIsNeg;

    while (isspace (*pstr))
        ++pstr;

    cCurr = *pstr++;
    iIsNeg = cCurr;
    if (('-' == cCurr) || ('+' == cCurr))
        cCurr = *pstr++;

    lTotal = 0;

    while (isdigit(cCurr)) {
        lTotal = 10 * lTotal + (cCurr - '0');
        cCurr = *pstr++;
    }

    if ('-' == iIsNeg)
        return (-lTotal);
    else
        return (lTotal);
}

int __cdecl _ttoi(const char * pstr)
{
    return ((int)_ttol (pstr));
}
*/
/*
int __cdecl tolower(int c)
{
	if (c - 'A' + 'a' < 0)
		return c;
	else
		return c - 'A' + 'a';
}
*/
/*
char* __cdecl strchr(const char* str, int ch)
{
	while (*str != '\0') {
		if (*str == ch)
			return (char*)str;
		str++;
	}
	return NULL;
}

char* __cdecl strrchr(const char* str, int ch)
{
	char* strb = (char*)str;

	while (*str != '\0') {
		str++;
	}
	while (str != strb) {
		if (*str == ch)
			return (char*)str;
		str--;
	}
	return NULL;
}
*/
