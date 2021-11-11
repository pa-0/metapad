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

#ifndef FILE_LOAD_H
#define FILE_LOAD_H

#ifdef USE_RICH_EDIT
int FixTextBuffer(LPTSTR szText);
#else
void FixTextBufferLE(LPTSTR* pszBuffer);
#endif

void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU);
void LoadFileFromMenu(WORD wMenu, BOOL bMRU);
DWORD LoadFileIntoBuffer(HANDLE hFile, LPBYTE* ppBuffer, ULONG* plBufferLength, INT* pnFileEncoding);

#endif
