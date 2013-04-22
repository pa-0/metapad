///// Consts /////
#ifndef CONSTS_H
#define CONSTS_H

#define MAXFN 300
#define MAXFONT 100
#define MAXARGS 100
#define MAXQUOTE 100
#define MAXFIND 100
#define NUMPANES 5
#define MAXFAVESIZE 2000
#define MAXSTRING 500
//#define MAXFAVES 16
#define MAXMACRO 1001

#define NUMCUSTOMBITMAPS 6
#ifdef USE_RICH_EDIT
#define NUMBUTTONS 26
#else
#define NUMBUTTONS 25
#endif

#define CUSTOMBMPBASE 15
#define NUMFINDS 10
#define EGGNUM 15
#define SBPANE_TYPE 0
#define SBPANE_INS 1
#define SBPANE_LINE 2
#define SBPANE_COL 3
#define SBPANE_MESSAGE 4
#define STATUS_FONT_CONST 1.4

#ifdef USE_RICH_EDIT
#define RECENTPOS (options.bReadOnlyMenu ? 15 : 14)
#define CONVERTPOS 8
#else
#define RECENTPOS (options.bReadOnlyMenu ? 14 : 13)
#define CONVERTPOS 7
#endif

#define FILEFORMATPOS (options.bReadOnlyMenu ? 8 : 7)
#define EDITPOS (options.bRecentOnOwn ? 2 : 1)
#define READONLYPOS 4
#define FAVEPOS (options.bRecentOnOwn ? 3 : 2)

#define ID_CLIENT 100
#define ID_STATUSBAR 101
#define ID_TOOLBAR 102

#define FILE_FORMAT_DOS 0
#define FILE_FORMAT_UNIX 1
#define FILE_FORMAT_UNICODE 2
#define FILE_FORMAT_UNICODE_BE 3
#define FILE_FORMAT_UTF_8 4

#define SIZEOFBOM_UTF_8 3
#define SIZEOFBOM_UTF_16 2

#define TYPE_UNKNOWN 0
#define TYPE_UTF_8 1
#define TYPE_UTF_16 2
#define TYPE_UTF_16_BE 3

#define WS_EX_LAYERED 0x00080000
#define LWA_COLORKEY 0x00000001
#define LWA_ALPHA 0x00000002

#endif

