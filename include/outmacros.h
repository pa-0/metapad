#ifndef OUTMACROS_H
#define OUTMACROS_H

#define ERROROUT(_x) MessageBox(hwnd, _x, STR_METAPAD, MB_OK | MB_ICONEXCLAMATION)
#define MSGOUT(_x) MessageBox(hwnd, _x, STR_METAPAD, MB_OK | MB_ICONINFORMATION)
#define DBGOUT(_x, _y) MessageBox(hwnd, _x, _y, MB_OK | MB_ICONEXCLAMATION)

#endif
