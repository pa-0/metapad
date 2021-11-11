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

#ifndef TMP_PROTOS_H
#define TMP_PROTOS_H

///// Prototypes /////

LPTSTR GetString(UINT uID);
BOOL GetCheckedState(HMENU hmenu, UINT nID, BOOL bToggle);
void CreateClient(HWND hParent, LPCTSTR szText, BOOL bWrap);
BOOL CALLBACK AbortDlgProc(HDC hdc, int nCode);
LRESULT CALLBACK AbortPrintJob(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
void PrintContents(void);
void ReportError(UINT);
void ReportLastError(void);
void CenterWindow(HWND hwndCenter);
void SelectWord(LPTSTR* target, BOOL bSmart, BOOL bAutoSelect);
void SetFont(HFONT* phfnt, BOOL bPrimary);
void SetTabStops(void);
//void NextWord(BOOL bRight, BOOL bSelect); // Uninmplemented.
void UpdateStatus(void);
BOOL SetClientFont(BOOL bPrimary);
BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Advanced2PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ViewPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI MainWndProc(HWND hwndMain, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT APIENTRY EditProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateMRUList(void);
void SaveMRUInfo(LPCTSTR szFullPath);
void SwitchReadOnly(BOOL bNewVal);
BOOL EncodeWithEscapeSeqs(TCHAR* szText);
BOOL ParseForEscapeSeqs(LPTSTR buf, LPBYTE* specials, LPCTSTR errContext);
void ReverseBytes(LPBYTE buffer, LONG size);
void UpdateCaption(void);

#ifdef _DEBUG
#include <stdio.h>
#endif

#endif
