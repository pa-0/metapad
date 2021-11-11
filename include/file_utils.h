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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

long CalculateFileSize(void);
void ExpandFilename(LPCTSTR szBuffer, LPTSTR* szOut);
BOOL SearchFile(LPCTSTR szText, BOOL bCase, BOOL bReplaceAll, BOOL bDown, BOOL bWholeWord);
DWORD ReplaceAll(LPTSTR* szBuf, DWORD* bufLen, LPCTSTR szFind, LPCTSTR szRepl, BOOL bCase, BOOL bWholeWord);
void SetFileFormat(int nFormat);

#endif
