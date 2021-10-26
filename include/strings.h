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

#ifndef STRINGS_H
#define STRINGS_H

#ifdef BUILD_METAPAD_UNICODE
#ifdef USE_RICH_EDIT
#define STR_ABOUT_NORMAL _T("metapad 3.7")
#else
#define STR_ABOUT_NORMAL _T("metapad LE 3.7")
#endif
#else
#ifdef USE_RICH_EDIT
#define STR_ABOUT_NORMAL _T("metapad 3.7-ANSI")
#else
#define STR_ABOUT_NORMAL _T("metapad LE 3.7-ANSI")
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
