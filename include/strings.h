#ifndef STRINGS_H
#define STRINGS_H

#ifdef BUILD_METAPAD_UNICODE
#ifdef USE_RICH_EDIT
#define STR_ABOUT_NORMAL _T("metapad 3.xU ALPHA 0")
#else
#define STR_ABOUT_NORMAL _T("metapad LE 3.xU ALPHA 0")
#endif
#else
#ifdef USE_RICH_EDIT
#define STR_ABOUT_NORMAL _T("metapad 3.6")
#else
#define STR_ABOUT_NORMAL _T("metapad LE 3.6")
#endif
#endif

#define STR_ABOUT_HACKER _T("m374p4d i5 d4 5hi7!")

#ifdef USE_RICH_EDIT
#define STR_RICHDLL _T("RICHED20.DLL")
#endif

#define STR_METAPAD _T("metapad")
#define STR_FAV_FILE _T("metafav.ini")
#define STR_CAPTION_FILE _T("%s - metapad")
#define STR_URL _T("http://liquidninja.com/metapad")
#define STR_REGKEY _T("SOFTWARE\\metapad")
#define STR_FAV_APPNAME _T("Favourites")
#define STR_COPYRIGHT _T("© 1999-2011 Alexander Davidson")

#endif
