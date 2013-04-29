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

#ifndef TMP_PROTOS_H
#define TMP_PROTOS_H

///// Prototypes /////

LPTSTR GetString(UINT uID);
void MakeNewFile(void);
BOOL SaveIfDirty(void);
BOOL GetCheckedState(HMENU hmenu, UINT nID, BOOL bToggle);
void CreateClient(HWND hParent, LPCTSTR szText, BOOL bWrap);
LPCTSTR GetShadowBuffer(void);
BOOL CALLBACK AbortDlgProc(HDC hdc, int nCode);
LRESULT CALLBACK AbortPrintJob(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoSearch(LPCTSTR szText, LONG lStart, LONG lEnd, BOOL bDown, BOOL bWholeWord, BOOL bCase, BOOL bFromTop);
void ExpandFilename(LPTSTR szBuffer);
void PrintContents(void);
void ReportLastError(void);
void LoadOptionString(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData);
void LoadBoundedOptionString(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData);
BOOL LoadOptionNumeric(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData);
void LoadOptionBinary(HKEY hKey, LPCSTR name, BYTE* lpData, DWORD cbData);
void LoadOptions(void);
void LoadWindowPlacement(int* left, int* top, int* width, int* height, int* nShow);
void CenterWindow(HWND hwndCenter);
void SelectWord(BOOL bFinding, BOOL bSmart, BOOL bAutoSelect);
void LoadFile(LPTSTR szFilename, BOOL bCreate, BOOL bMRU);
BOOL SaveFile(LPCTSTR szFilename);
void SetFont(HFONT* phfnt, BOOL bPrimary);
void SetTabStops(void);
void NextWord(BOOL bRight, BOOL bSelect);
void UpdateStatus(void);
BOOL SetClientFont(BOOL bPrimary);
BOOL SearchFile(LPCTSTR szText, BOOL bMatchCase, BOOL bReplaceAll, BOOL bDown, BOOL bWholeWord);
BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Advanced2PageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ViewPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LONG WINAPI MainWndProc(HWND hwndMain, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT APIENTRY EditProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateMRUList(void);
void SaveMRUInfo(LPCTSTR szFullPath);
void SwitchReadOnly(BOOL bNewVal);
BOOL EncodeWithEscapeSeqs(TCHAR* szText);

#endif
