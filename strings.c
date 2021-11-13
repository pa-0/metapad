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

#include "include/resource.h"
#include "include/globals.h"
#include "include/macros.h"

#define NUMSTRINGS 246
static const CHAR strings[] = ""
/*	`	1	`	IDS_STATFMT_BYTES				*/	"\0 Bytes: %d "
/*	`	2	`	IDS_STATFMT_SEL					*/	"\0 Selected %d:%d  ->  %d:%d  %s"
/*	`	3	`	IDS_STATFMT_LINE				*/	"\0 Line: %d/%d"
/*	`	4	`	IDS_STATFMT_COL					*/	"\0 Col: %d"
#ifdef USE_RICH_EDIT
/*	`	5	`	IDS_STATFMT_INS					*/	"\0 INS"
/*	`	6	`	IDS_STATFMT_OVR					*/	"\0OVR"
#else
"\0\0"
#endif
/*	`	7	`	IDS_FILTER_EXEC					*/	"\0Executable Files (*.exe)|*.exe|All Files (*.*)|*.*|"
/*	`	8	`	IDS_FILTER_PLUGIN				*/	"\0metapad language plugins (*.dll)|*.dll|All Files (*.*)|*.*|"
/*	`	9	`	IDS_DEFAULT_FILTER				*/	"\0All Textual File Types|*.txt;*.htm;*.html;*.c;*.cpp;*.h;*.java|Text Files (*.txt)|*.txt|HTML (*.html; *.htm)|*.html;*.htm|Source Code (*.c; *.cpp; *.h; *.java)|*.c;*.cpp;*.h;*.java|All Files (*.*)|*.*|"
/*	`	10	`	IDS_DEFAULT_FILTER_TEXT			*/	"\0Text (*.*)|*.*|"
/*	`	11	`	STR_METAPAD						*/	"\0metapad"
/*	`	12	`	STR_CAPTION_FILE				*/	"\0 - metapad "
#ifdef UNICODE
#ifdef USE_RICH_EDIT
/*	`	13	`	STR_ABOUT						*/	"\0metapad 3.7"
#else
/*	`	13	`	STR_ABOUT						*/	"\0metapad LE 3.7"
#endif
#else
#ifdef USE_RICH_EDIT
/*	`	13	`	STR_ABOUT						*/	"\0metapad 3.7-ANSI"
#else
/*	`	13	`	STR_ABOUT						*/	"\0metapad LE 3.7-ANSI"
#endif
#endif
/*	`	14	`	STR_FAV_FILE					*/	"\0metafav.ini"
/*	`	15	`	STR_INI_FILE					*/	"\0metapad.ini"
/*	`	16	`	STR_URL							*/	"\0http://liquidninja.com/metapad"
/*	`	17	`	STR_REGKEY						*/	"\0SOFTWARE\\metapad"
/*	`	18	`	STR_FAV_APPNAME					*/	"\0Favourites"
/*	`	19	`	STR_OPTIONS						*/	"\0Options"
/*	`	20	`	STR_COPYRIGHT					*/	"\0\xa9 1999-2011 Alexander Davidson\n\xa9 2013 Mario Rugiero\n\xa9 2021 SoBiT Corp"
/*	`	21	`	IDS_MIGRATED					*/	"\0Migration to INI completed."
/*	`	22	`	IDS_DECODEBASE_BADLEN			*/	"\0Invalid code string length!"
/*	`	23	`	IDS_DECODEBASE_BADCHAR			*/	"\0Invalid code characters!"
/*	`	24	`	IDS_PLUGIN_ERRFIND				*/	"\0Could not find the language plugin DLL."
/*	`	25	`	IDS_PLUGIN_ERR					*/	"\0Temporarily reverting language to Default (English)\n\nCheck the language plugin setting."
/*	`	26	`	ID_LFMT_DOS						*/	"\0DOS"
/*	`	27	`	ID_LFMT_UNIX					*/	"\0Unix"
/*	`	28	`	ID_LFMT_MAC						*/	"\0Mac"
/*	`	29	`	ID_LFMT_MIXED					*/	"\0RAW"
/*	`	30	`	ID_ENC_ANSI						*/	"\0ANSI"
/*	`	31	`	ID_ENC_UTF8						*/	"\0UTF-8"
/*	`	32	`	ID_ENC_UTF16					*/	"\0Unicode"
/*	`	33	`	ID_ENC_UTF16BE					*/	"\0Unicode BE"
/*	`	34	`	ID_ENC_BIN						*/	"\0Binary"
/*	`	35	`	ID_ENC_CUSTOM					*/	"\0CP%d"
/*	`	36	`	IDS_ENC_CUSTOM_CAPTION			*/	"\0Codepage %d"
/*	`	37	`									*/	"\0"
/*	`	38	`									*/	"\0"
/*	`	39	`									*/	"\0"
/*	`	40	`	IDSS_WSTATE						*/	"\0w_WindowState"
/*	`	41	`	IDSS_WLEFT						*/	"\0w_Left"
/*	`	42	`	IDSS_WTOP						*/	"\0w_Top"
/*	`	43	`	IDSS_WWIDTH						*/	"\0w_Width"
/*	`	44	`	IDSS_WHEIGHT					*/	"\0w_Height"
/*	`	45	`	IDSS_MRU						*/	"\0mru_%d"
/*	`	46	`	IDSS_MRUTOP						*/	"\0mru_Top"
/*	`	47	`	IDSS_HIDEGOTOOFFSET				*/	"\0bHideGotoOffset"
/*	`	48	`	IDSS_SYSTEMCOLOURS				*/	"\0bSystemColours"
/*	`	49	`	IDSS_SYSTEMCOLOURS2				*/	"\0bSystemColours2"
/*	`	50	`	IDSS_NOSMARTHOME				*/	"\0bNoSmartHome"
/*	`	51	`	IDSS_NOAUTOSAVEEXT				*/	"\0bNoAutoSaveExt"
/*	`	52	`	IDSS_CONTEXTCURSOR				*/	"\0bContextCursor"
/*	`	53	`	IDSS_CURRENTFINDFONT			*/	"\0bCurrentFindFont"
/*	`	54	`	IDSS_PRINTWITHSECONDARYFONT		*/	"\0bPrintWithSecondaryFont"
/*	`	55	`	IDSS_NOSAVEHISTORY				*/	"\0bNoSaveHistory"
/*	`	56	`	IDSS_NOFINDAUTOSELECT			*/	"\0bNoFindAutoSelect"
/*	`	57	`	IDSS_RECENTONOWN				*/	"\0bRecentOnOwn"
/*	`	58	`	IDSS_DONTINSERTTIME				*/	"\0bDontInsertTime"
/*	`	59	`	IDSS_NOWARNINGPROMPT			*/	"\0bNoWarningPrompt"
/*	`	60	`	IDSS_UNFLATTOOLBAR				*/	"\0bUnFlatToolbar"
/*	`	61	`	IDSS_STICKYWINDOW				*/	"\0bStickyWindow"
/*	`	62	`	IDSS_READONLYMENU				*/	"\0bReadOnlyMenu"
/*	`	63	`	IDSS_SELECTIONMARGINWIDTH		*/	"\0nSelectionMarginWidth"
/*	`	64	`	IDSS_MAXMRU						*/	"\0nMaxMRU"
/*	`	65	`	IDSS_FORMAT						*/	"\0nFormat"
/*	`	66	`	IDSS_TRANSPARENTPCT				*/	"\0nTransparentPct"
/*	`	67	`	IDSS_NOCAPTIONDIR				*/	"\0bNoCaptionDir"
/*	`	68	`	IDSS_AUTOINDENT					*/	"\0bAutoIndent"
/*	`	69	`	IDSS_INSERTSPACES				*/	"\0bInsertSpaces"
/*	`	70	`	IDSS_FINDAUTOWRAP				*/	"\0bFindAutoWrap"
/*	`	71	`	IDSS_QUICKEXIT					*/	"\0bQuickExit"
/*	`	72	`	IDSS_SAVEWINDOWPLACEMENT		*/	"\0bSaveWindowPlacement"
/*	`	73	`	IDSS_SAVEMENUSETTINGS			*/	"\0bSaveMenuSettings"
/*	`	74	`	IDSS_SAVEDIRECTORY				*/	"\0bSaveDirectory"
/*	`	75	`	IDSS_LAUNCHCLOSE				*/	"\0bLaunchClose"
/*	`	76	`	IDSS_NOFAVES					*/	"\0bNoFaves"
#ifdef USE_RICH_EDIT
/*	`	77	`	IDSS_DEFAULTPRINTFONT			*/	"\0bDefaultPrintFont"
/*	`	78	`	IDSS_ALWAYSLAUNCH				*/	"\0bAlwaysLaunch"
"\0\0\0"
#else
"\0\0"
/*	`	79	`	IDSS_LINKDOUBLECLICK			*/	"\0bLinkDoubleClick"
/*	`	80	`	IDSS_HIDESCROLLBARS				*/	"\0bHideScrollbars"
/*	`	81	`	IDSS_SUPPRESSUNDOBUFFERPROMPT	*/	"\0bSuppressUndoBufferPrompt"
#endif
/*	`	82	`	IDSS_LAUNCHSAVE					*/	"\0nLaunchSave"
/*	`	83	`	IDSS_TABSTOPS					*/	"\0nTabStops"
/*	`	84	`	IDSS_NPRIMARYFONT				*/	"\0nPrimaryFont"
/*	`	85	`	IDSS_NSECONDARYFONT				*/	"\0nSecondaryFont"
#ifdef UNICODE
/*	`	86	`	IDSS_PRIMARYFONT				*/	"\0PrimaryFontU"
/*	`	87	`	IDSS_SECONDARYFONT				*/	"\0SecondaryFontU"
#else
/*	`	86	`	IDSS_PRIMARYFONT				*/	"\0PrimaryFont"
/*	`	87	`	IDSS_SECONDARYFONT				*/	"\0SecondaryFont"
#endif
/*	`	88	`	IDSS_BROWSER					*/	"\0szBrowser"
/*	`	89	`	IDSS_BROWSER2					*/	"\0szBrowser2"
/*	`	90	`	IDSS_LANGPLUGIN					*/	"\0szLangPlugin"
/*	`	91	`	IDSS_FAVDIR						*/	"\0szFavDir"
/*	`	92	`	IDSS_ARGS						*/	"\0szArgs"
/*	`	93	`	IDSS_ARGS2						*/	"\0szArgs2"
/*	`	94	`	IDSS_QUOTE						*/	"\0szQuote"
/*	`	95	`	IDSS_CUSTOMDATE					*/	"\0szCustomDate"
/*	`	96	`	IDSS_CUSTOMDATE2				*/	"\0szCustomDate2"
/*	`	97	`	IDSS_MACROARRAY					*/	"\0szMacroArray%d"
/*	`	98	`	IDSS_BACKCOLOUR					*/	"\0BackColour"
/*	`	99	`	IDSS_FONTCOLOUR					*/	"\0FontColour"
/*	`	100	`	IDSS_BACKCOLOUR2				*/	"\0BackColour2"
/*	`	101	`	IDSS_FONTCOLOUR2				*/	"\0FontColour2"
/*	`	102	`	IDSS_MARGINS					*/	"\0rMargins"
/*	`	103	`	IDSS_WORDWRAP					*/	"\0m_WordWrap"
/*	`	104	`	IDSS_FONTIDX					*/	"\0m_PrimaryFont"
/*	`	105	`	IDSS_SMARTSELECT				*/	"\0m_SmartSelect"
#ifdef USE_RICH_EDIT
/*	`	106	`	IDSS_HYPERLINKS					*/	"\0m_Hyperlinks"
#else
"\0"
#endif
/*	`	107	`	IDSS_SHOWSTATUS					*/	"\0m_ShowStatus"
/*	`	108	`	IDSS_SHOWTOOLBAR				*/	"\0m_ShowToolbar"
/*	`	109	`	IDSS_ALWAYSONTOP				*/	"\0m_AlwaysOnTop"
/*	`	110	`	IDSS_TRANSPARENT				*/	"\0m_Transparent"
/*	`	111	`	IDSS_CLOSEAFTERFIND				*/	"\0bCloseAfterFind"
/*	`	112	`	IDSS_CLOSEAFTERREPLACE			*/	"\0bCloseAfterReplace"
/*	`	113	`	IDSS_CLOSEAFTERINSERT			*/	"\0bCloseAfterInsert"
/*	`	114	`	IDSS_NOFINDHIDDEN				*/	"\0bNoFindHidden"
/*	`	115	`	IDSS_FILEFILTER					*/	"\0FileFilter"
/*	`	116	`	IDSS_FINDARRAY					*/	"\0szFindArray%d"
/*	`	117	`	IDSS_REPLACEARRAY				*/	"\0szReplaceArray%d"
/*	`	118	`	IDSS_INSERTARRAY				*/	"\0szInsertArray%d"
/*	`	119	`	IDSS_LASTDIRECTORY				*/	"\0szLastDirectory"
/*	`	120	`									*/	"\0"
/*	`	121	`	IDSD_QUOTE						*/	"\0> "
/*	`	122	`	IDSD_CUSTOMDATE					*/	"\0yyyyMMdd-HHmmss "
/*	`	123	`									*/	"\0"
/*	`	124	`									*/	"\0"
/*	`	125	`	IDS_VERSION_SYNCH				*/	"\0""3.7"
/*	`	126	`	IDS_PLUGIN_LANGUAGE				*/	"\0"
/*	`	127	`	IDS_PLUGIN_RELEASE				*/	"\0"
/*	`	128	`	IDS_PLUGIN_TRANSLATOR			*/	"\0"
/*	`	129	`	IDS_PLUGIN_EMAIL				*/	"\0"
/*	`	130	`	IDS_DIRTYFILE					*/	"\0Save changes to %s?"
/*	`	131	`	IDS_ERROR_LOCKED				*/	"\0Error writing to file - it may be locked or read only."
/*	`	132	`	IDS_ERROR_SEARCH				*/	"\0Search string not found."
/*	`	133	`	IDS_LAUNCH_WARNING				*/	"\0Warning - you have chosen not to save AND to close metapad on external launch.\n\nThis could cause you to lose data!"
/*	`	134	`	IDS_READONLY_INDICATOR			*/	"\0(read only)"
/*	`	135	`	IDS_READONLY_MENU				*/	"\0R&ead Only\tCtrl+E"
/*	`	136	`	IDS_ERROR_FAVOURITES			*/	"\0Error in favorites file. It may have been modified and is not synchronized."
/*	`	137	`	IDS_FILE_LOADING				*/	"\0Loading file..."
/*	`	138	`	IDS_READONLY_WARNING			*/	"\0This file is read only. You must save to a different file."
/*	`	139	`	IDS_PRINTER_NOT_FOUND			*/	"\0Printer not found!\n(Perhaps try reinstalling the printer.)"
/*	`	140	`	IDS_PRINT_INIT_ERROR			*/	"\0Error initializing printing. (CommDlgExtendedError: %#04x)"
/*	`	141	`	IDS_PRINT_ABORT_ERROR			*/	"\0Error aborting printing"
/*	`	142	`	IDS_PRINT_START_ERROR			*/	"\0StartDoc printing error. Printing canceled."
/*	`	143	`	IDS_DRAWTEXT_ERROR				*/	"\0Error calling DrawText."
/*	`	144	`	IDS_PRINT_ERROR					*/	"\0There was an error during printing. Print canceled."
/*	`	145	`	IDS_VIEWER_ERROR				*/	"\0Error launching external viewer"
/*	`	146	`	IDS_VIEWER_MISSING				*/	"\0External viewer is not configured."
/*	`	147	`	IDS_NO_DEFAULT_VIEWER			*/	"\0No viewer associated with this file type."
/*	`	148	`	IDS_REGISTRY_WINDOW_ERROR		*/	"\0Missing window placement registry values - using default settings."
/*	`	149	`	IDS_CREATE_FILE_MESSAGE			*/	"\0\"%s\" not found. Create it?"
/*	`	150	`	IDS_FILE_CREATE_ERROR			*/	"\0File creation error."
/*	`	151	`	IDS_FILE_NOT_FOUND				*/	"\0File not found."
/*	`	152	`	IDS_FILE_LOCKED_ERROR			*/	"\0File is locked by another process."
#ifdef UNICODE
"\0\0\0\0"
#else
/*	`	153	`	IDS_UNICODE_CONVERT_ERROR		*/	"\0Error converting from Unicode file"
/*	`	154	`	IDS_UNICODE_CHARS_WARNING		*/	"\0Detected non-ANSI characters in this Unicode file.\n\nData will be lost if this file is saved!"
/*	`	155	`	IDS_BINARY_FILE_WARNING			*/	"\0This is a binary file.\nConvert any NULL terminators to spaces and load anyway?"
/*	`	156	`	IDS_UNICODE_STRING_ERROR		*/	"\0Error converting to Unicode string"
#endif
/*	`	157	`	IDS_WRITE_BOM_ERROR				*/	"\0Error writing BOM"
/*	`	158	`	IDS_CHAR_WIDTH_ERROR			*/	"\0Couldn't get char width."
/*	`	159	`	IDS_PARA_FORMAT_ERROR			*/	"\0Couldn't set para format."
/*	`	160	`	IDS_QUERY_SEARCH_TOP			*/	"\0Reached end of file. Continue searching from the top?"
/*	`	161	`	IDS_QUERY_SEARCH_BOTTOM			*/	"\0Reached beginning of file. Continue searching from the bottom?"
/*	`	162	`	IDS_RECENT_MENU					*/	"\0&Recent"
/*	`	163	`	IDS_RECENT_FILES_MENU			*/	"\0Recent &Files"
/*	`	164	`	IDS_MAX_RECENT_WARNING			*/	"\0Enter a maximum Recent File size between 0 and 16"
/*	`	165	`	IDS_STICKY_MESSAGE				*/	"\0Main window will now always start up with its current size and position."
/*	`	166	`	IDS_CLEAR_FIND_WARNING			*/	"\0This action will clear find, replace, and insert histories."
/*	`	167	`	IDS_CLEAR_RECENT_WARNING		*/	"\0This action will clear the recent files history"
/*	`	168	`	IDS_SELECT_PLUGIN_WARNING		*/	"\0Select a language plugin or choose Default (English)."
/*	`	169	`	IDS_MARGIN_WIDTH_WARNING		*/	"\0Enter a margin width between 0 and 300"
/*	`	170	`	IDS_TRANSPARENCY_WARNING		*/	"\0Enter a transparency mode percent between 1 and 99"
/*	`	171	`	IDS_TAB_SIZE_WARNING			*/	"\0Enter a tab size between 1 and 100"
/*	`	172	`	IDS_TB_NEWFILE					*/	"\0New File"
/*	`	173	`	IDS_TB_OPENFILE					*/	"\0Open File"
/*	`	174	`	IDS_TB_SAVEFILE					*/	"\0Save File"
/*	`	175	`	IDS_TB_PRINT					*/	"\0Print"
/*	`	176	`	IDS_TB_FIND						*/	"\0Find"
/*	`	177	`	IDS_TB_REPLACE					*/	"\0Replace"
/*	`	178	`	IDS_TB_CUT						*/	"\0Cut"
/*	`	179	`	IDS_TB_COPY						*/	"\0Copy"
/*	`	180	`	IDS_TB_PASTE					*/	"\0Paste"
/*	`	181	`	IDS_TB_UNDO						*/	"\0Undo"
/*	`	182	`	IDS_TB_SETTINGS					*/	"\0Settings"
/*	`	183	`	IDS_TB_REFRESH					*/	"\0Refresh/Revert"
/*	`	184	`	IDS_TB_WORDWRAP					*/	"\0Word Wrap"
/*	`	185	`	IDS_TB_PRIMARYFONT				*/	"\0Primary Font"
/*	`	186	`	IDS_TB_ONTOP					*/	"\0Always On Top"
/*	`	187	`	IDS_TB_PRIMARYVIEWER			*/	"\0Launch Primary Viewer"
/*	`	188	`	IDS_TB_SECONDARYVIEWER			*/	"\0Launch Secondary Viewer"
#ifdef USE_RICH_EDIT
/*	`	189	`	IDS_TB_REDO						*/	"\0Redo"
/*	`	190	`	IDS_CANT_UNDO_WARNING			*/	"\0Can't undo this action!"
/*	`	191	`	IDS_MEMORY_LIMIT				*/	"\0Reached memory limit at %d bytes!"
/*	`	192	`	IDS_FONT_UNDO_WARNING			*/	"\0Applying new font settings. This action will empty the undo buffer."
/*	`	193	`	IDS_UNDO_HYPERLINKS_WARNING		*/	"\0Updating hyperlinks. This action will empty the undo buffer."
/*	`	194	`	IDS_RICHED_MISSING_ERROR		*/	"\0metapad requires RICHED20.DLL to be installed on your system.\n\nSee the FAQ on the website for further information."
/*	`	195	`	IDS_RESTART_HIDE_SB				*/	"\0The change in \"Hide scrollbars when possible\" will be reflected upon restarting the program."
"\0\0"
#else
"\0\0\0\0\0\0\0"
/*	`	196	`	IDS_QUERY_LAUNCH_VIEWER			*/	"\0This version of Windows does not support loading files this large into an Edit window.\nLaunch in external viewer?"
/*	`	197	`	IDS_LE_MEMORY_LIMIT				*/	"\0Reached memory limit!\nSave file now and use another editor (metapad full not LE)!"
#endif
/*	`	198	`	IDS_CHANGE_READONLY_ERROR		*/	"\0Unable to change the readonly attribute for this file."
/*	`	199	`	IDS_SETTINGS_TITLE				*/	"\0Settings"
/*	`	200	`	IDS_NO_SELECTED_TEXT			*/	"\0No selected text."
/*	`	201	`	IDS_ITEMS_REPLACED				*/	"\0Replaced %d item(s)."
/*	`	202	`	IDS_MENU_LANGUAGE_PLUGIN		*/	"\0&Language Plugin..."
/*	`	203	`	IDS_COMMAND_LINE_OPTIONS		*/	"\0Usage: metapad [/i | /m | /v | /s] [/p | /g row:col | /e] [filename]\n\nOption descriptions:\n\n\t/i - force INI mode for settings\n\t/m - migrate all registry settings to INI file\n\t/g row:col - goto position at row and column\n\t/e - goto end of file\n\t/p - print file\n\t/v - disable plugin version checking\n\t/s - skip loading language plugin"
/*	`	204	`	IDS_GLOBALLOCK_ERROR			*/	"\0GlobalLock failed"
/*	`	205	`	IDS_CLIPBOARD_UNLOCK_ERROR		*/	"\0There was an error unlocking the clipboard memory."
/*	`	206	`	IDS_FILE_READ_ERROR				*/	"\0Error reading file."
/*	`	207	`	IDS_RESTART_FAVES				*/	"\0The change in \"Disable Favorites\" will be reflected upon restarting the program."
/*	`	208	`	IDS_RESTART_LANG				*/	"\0Language changes will be reflected upon restarting the program."
/*	`	209	`	IDS_INVALID_PLUGIN_ERROR		*/	"\0Error loading language plugin DLL. It is probably not a valid metapad language plugin."
/*	`	210	`	IDS_BAD_STRING_PLUGIN_ERROR		*/	"\0Error fetching version string resource from plugin DLL. It is probably not a valid metapad language plugin."
/*	`	211	`	IDS_PLUGIN_MISMATCH_ERROR		*/	"\0Your language plugin is for metapad %s, but you are running metapad %s.\n\nSome menu or dialog features may be missing or not work correctly.\n\nRun metapad /v to skip this message."
/*	`	212	`	IDS_ALLRIGHTS					*/	"\0All Rights Reserved"
/*	`	213	`	IDS_OK							*/	"\0OK"
/*	`	214	`	IDS_NEW_FILE					*/	"\0new file"
/*	`	215	`	IDS_MACRO_LENGTH_ERROR			*/	"\0Selection is larger than maximum quick buffer length."
/*	`	216	`	IDS_ERROR						*/	"\0Error"
/*	`	217	`	IDS_ERROR_MSG					*/	"\0\nError 0x%08X"
/*	`	218	`	IDS_UNICODE_SAVE_TRUNCATION		*/	"\0Some characters in this text cannot be saved in the selected encoding!\n\nWould you like to save this file as UTF-8?"
/*	`	219	`	IDS_UNICODE_LOAD_ERROR			*/	"\0Error converting file to Unicode"
/*	`	220	`	IDS_UNICODE_LOAD_TRUNCATION		*/	"\0Detected unsupported or invalid code points.\n\nData will be lost if this file is saved!"
/*	`	221	`	IDS_BINARY_FILE_WARNING_SAFE	*/	"\0This is a binary file. Load anyway?\nNull terminators will be preserved unless the File Format is changed."
/*	`	222	`	IDS_LARGE_FILE_WARNING			*/	"\0This file is very large ( >16 MB). Loading it may take a long time.\nLoad anyway?"
/*	`	223	`	IDS_UNICODE_CONV_ERROR			*/	"\0Error converting string to Unicode"
/*	`	224	`	IDS_ESCAPE_ERROR				*/	"\0%s in '%s' near:\n"
/*	`	225	`	IDS_ESCAPE_BADCHARS				*/	"\0Invalid characters in base-%d string"
/*	`	226	`	IDS_ESCAPE_BADALIGN				*/	"\0The base-%d string is not a multiple of %d characters"
/*	`	227	`	IDS_ESCAPE_EXPECTED				*/	"\0Expected base-%d string"
/*	`	228	`	IDS_ESCAPE_CTX_FIND				*/	"\0Find what"
/*	`	229	`	IDS_ESCAPE_CTX_REPLACE			*/	"\0Replace with"
/*	`	230	`	IDS_ESCAPE_CTX_INSERT			*/	"\0Insert Text"
/*	`	231	`	IDS_ESCAPE_CTX_CLIPBRD			*/	"\0Clipboard"
/*	`	232	`	IDS_ESCAPE_CTX_MACRO			*/	"\0Macro"
/*	`	233	`	IDS_ESCAPE_CTX_QUOTE			*/	"\0Quote string"
/*	`	234	`	IDS_LARGE_PASTE_WARNING			*/	"\0About to insert %I64u characters of text.\nThis may take a very long time!\nContinue?"
/*	`	235	`	IDS_ITEMS_REPLACED_ITER			*/	"\0Replaced %d item(s) in %d iteration(s)."
/*	`	236	`	IDS_ENC_REINTERPRET				*/	"\0Reinterpret the current text as %s?"
/*	`	237	`	IDS_LFMT_MIXED					*/	"\0This text has mixed line endings.\nYou can normalize the line endings in the File > Format menu."
/*	`	238	`	IDS_LFMT_FIXED					*/	"\0Mixed line endings have been normalized.\nYou can change this function in the File > Format menu."
/*	`	239	`	IDS_SETDEF_FORMAT_WARN			*/	"\0Note that all files without a Byte-Order Mark will now be interpreted as %s!"
/*	`	240	`	IDS_ENC_BAD						*/	"\0This codepage is not available on your system."
/*	`	241	`	IDS_ENC_FAILED					*/	"\0Error converting to the selected codepage!\nThe data was instead saved as ANSI."
/*	`	242	`									*/	"\0"
/*	`	243	`									*/	"\0"
/*	`	244	`									*/	"\0"
/*	`	245	`									*/	"\0"
"";
/* Excel:
	Import: '\n#'->'~#'  '\n"'->'~"'
	Export:
*/


static WORD stringsidx[NUMSTRINGS] = {
0,
IDS_STATFMT_BYTES,
IDS_STATFMT_SEL,
IDS_STATFMT_LINE,
IDS_STATFMT_COL,
IDS_STATFMT_INS,
IDS_STATFMT_OVR,
IDS_FILTER_EXEC,
IDS_FILTER_PLUGIN,
IDS_DEFAULT_FILTER,
IDS_DEFAULT_FILTER_TEXT,
STR_METAPAD,
STR_CAPTION_FILE,
STR_ABOUT,
STR_FAV_FILE,
STR_INI_FILE,
STR_URL,
STR_REGKEY,
STR_FAV_APPNAME,
STR_OPTIONS,
STR_COPYRIGHT,
IDS_MIGRATED,
IDS_DECODEBASE_BADLEN,
IDS_DECODEBASE_BADCHAR,
IDS_PLUGIN_ERRFIND,
IDS_PLUGIN_ERR,
ID_LFMT_DOS,
ID_LFMT_UNIX,
ID_LFMT_MAC,
ID_LFMT_MIXED,
ID_ENC_ANSI,
ID_ENC_UTF8,
ID_ENC_UTF16,
ID_ENC_UTF16BE,
ID_ENC_BIN,
ID_ENC_CUSTOM,
IDS_ENC_CUSTOM_CAPTION,
0,
0,
0,
IDSS_WSTATE,
IDSS_WLEFT,
IDSS_WTOP,
IDSS_WWIDTH,
IDSS_WHEIGHT,
IDSS_MRU,
IDSS_MRUTOP,
IDSS_HIDEGOTOOFFSET,
IDSS_SYSTEMCOLOURS,
IDSS_SYSTEMCOLOURS2,
IDSS_NOSMARTHOME,
IDSS_NOAUTOSAVEEXT,
IDSS_CONTEXTCURSOR,
IDSS_CURRENTFINDFONT,
IDSS_PRINTWITHSECONDARYFONT,
IDSS_NOSAVEHISTORY,
IDSS_NOFINDAUTOSELECT,
IDSS_RECENTONOWN,
IDSS_DONTINSERTTIME,
IDSS_NOWARNINGPROMPT,
IDSS_UNFLATTOOLBAR,
IDSS_STICKYWINDOW,
IDSS_READONLYMENU,
IDSS_SELECTIONMARGINWIDTH,
IDSS_MAXMRU,
IDSS_FORMAT,
IDSS_TRANSPARENTPCT,
IDSS_NOCAPTIONDIR,
IDSS_AUTOINDENT,
IDSS_INSERTSPACES,
IDSS_FINDAUTOWRAP,
IDSS_QUICKEXIT,
IDSS_SAVEWINDOWPLACEMENT,
IDSS_SAVEMENUSETTINGS,
IDSS_SAVEDIRECTORY,
IDSS_LAUNCHCLOSE,
IDSS_NOFAVES,
IDSS_DEFAULTPRINTFONT,
IDSS_ALWAYSLAUNCH,
IDSS_LINKDOUBLECLICK,
IDSS_HIDESCROLLBARS,
IDSS_SUPPRESSUNDOBUFFERPROMPT,
IDSS_LAUNCHSAVE,
IDSS_TABSTOPS,
IDSS_NPRIMARYFONT,
IDSS_NSECONDARYFONT,
IDSS_PRIMARYFONT,
IDSS_SECONDARYFONT,
IDSS_BROWSER,
IDSS_BROWSER2,
IDSS_LANGPLUGIN,
IDSS_FAVDIR,
IDSS_ARGS,
IDSS_ARGS2,
IDSS_QUOTE,
IDSS_CUSTOMDATE,
IDSS_CUSTOMDATE2,
IDSS_MACROARRAY,
IDSS_BACKCOLOUR,
IDSS_FONTCOLOUR,
IDSS_BACKCOLOUR2,
IDSS_FONTCOLOUR2,
IDSS_MARGINS,
IDSS_WORDWRAP,
IDSS_FONTIDX,
IDSS_SMARTSELECT,
IDSS_HYPERLINKS,
IDSS_SHOWSTATUS,
IDSS_SHOWTOOLBAR,
IDSS_ALWAYSONTOP,
IDSS_TRANSPARENT,
IDSS_CLOSEAFTERFIND,
IDSS_CLOSEAFTERREPLACE,
IDSS_CLOSEAFTERINSERT,
IDSS_NOFINDHIDDEN,
IDSS_FILEFILTER,
IDSS_FINDARRAY,
IDSS_REPLACEARRAY,
IDSS_INSERTARRAY,
IDSS_LASTDIRECTORY,
0,
IDSD_QUOTE,
IDSD_CUSTOMDATE,
0,
0,
IDS_VERSION_SYNCH,
IDS_PLUGIN_LANGUAGE,
IDS_PLUGIN_RELEASE,
IDS_PLUGIN_TRANSLATOR,
IDS_PLUGIN_EMAIL,
IDS_DIRTYFILE,
IDS_ERROR_LOCKED,
IDS_ERROR_SEARCH,
IDS_LAUNCH_WARNING,
IDS_READONLY_INDICATOR,
IDS_READONLY_MENU,
IDS_ERROR_FAVOURITES,
IDS_FILE_LOADING,
IDS_READONLY_WARNING,
IDS_PRINTER_NOT_FOUND,
IDS_PRINT_INIT_ERROR,
IDS_PRINT_ABORT_ERROR,
IDS_PRINT_START_ERROR,
IDS_DRAWTEXT_ERROR,
IDS_PRINT_ERROR,
IDS_VIEWER_ERROR,
IDS_VIEWER_MISSING,
IDS_NO_DEFAULT_VIEWER,
IDS_REGISTRY_WINDOW_ERROR,
IDS_CREATE_FILE_MESSAGE,
IDS_FILE_CREATE_ERROR,
IDS_FILE_NOT_FOUND,
IDS_FILE_LOCKED_ERROR,
IDS_UNICODE_CONVERT_ERROR,
IDS_UNICODE_CHARS_WARNING,
IDS_BINARY_FILE_WARNING,
IDS_UNICODE_STRING_ERROR,
IDS_WRITE_BOM_ERROR,
IDS_CHAR_WIDTH_ERROR,
IDS_PARA_FORMAT_ERROR,
IDS_QUERY_SEARCH_TOP,
IDS_QUERY_SEARCH_BOTTOM,
IDS_RECENT_MENU,
IDS_RECENT_FILES_MENU,
IDS_MAX_RECENT_WARNING,
IDS_STICKY_MESSAGE,
IDS_CLEAR_FIND_WARNING,
IDS_CLEAR_RECENT_WARNING,
IDS_SELECT_PLUGIN_WARNING,
IDS_MARGIN_WIDTH_WARNING,
IDS_TRANSPARENCY_WARNING,
IDS_TAB_SIZE_WARNING,
IDS_TB_NEWFILE,
IDS_TB_OPENFILE,
IDS_TB_SAVEFILE,
IDS_TB_PRINT,
IDS_TB_FIND,
IDS_TB_REPLACE,
IDS_TB_CUT,
IDS_TB_COPY,
IDS_TB_PASTE,
IDS_TB_UNDO,
IDS_TB_SETTINGS,
IDS_TB_REFRESH,
IDS_TB_WORDWRAP,
IDS_TB_PRIMARYFONT,
IDS_TB_ONTOP,
IDS_TB_PRIMARYVIEWER,
IDS_TB_SECONDARYVIEWER,
IDS_TB_REDO,
IDS_CANT_UNDO_WARNING,
IDS_MEMORY_LIMIT,
IDS_FONT_UNDO_WARNING,
IDS_UNDO_HYPERLINKS_WARNING,
IDS_RICHED_MISSING_ERROR,
IDS_RESTART_HIDE_SB,
IDS_QUERY_LAUNCH_VIEWER,
IDS_LE_MEMORY_LIMIT,
IDS_CHANGE_READONLY_ERROR,
IDS_SETTINGS_TITLE,
IDS_NO_SELECTED_TEXT,
IDS_ITEMS_REPLACED,
IDS_MENU_LANGUAGE_PLUGIN,
IDS_COMMAND_LINE_OPTIONS,
IDS_GLOBALLOCK_ERROR,
IDS_CLIPBOARD_UNLOCK_ERROR,
IDS_FILE_READ_ERROR,
IDS_RESTART_FAVES,
IDS_RESTART_LANG,
IDS_INVALID_PLUGIN_ERROR,
IDS_BAD_STRING_PLUGIN_ERROR,
IDS_PLUGIN_MISMATCH_ERROR,
IDS_ALLRIGHTS,
IDS_OK,
IDS_NEW_FILE,
IDS_MACRO_LENGTH_ERROR,
IDS_ERROR,
IDS_ERROR_MSG,
IDS_UNICODE_SAVE_TRUNCATION,
IDS_UNICODE_LOAD_ERROR,
IDS_UNICODE_LOAD_TRUNCATION,
IDS_BINARY_FILE_WARNING_SAFE,
IDS_LARGE_FILE_WARNING,
IDS_UNICODE_CONV_ERROR,
IDS_ESCAPE_ERROR,
IDS_ESCAPE_BADCHARS,
IDS_ESCAPE_BADALIGN,
IDS_ESCAPE_EXPECTED,
IDS_ESCAPE_CTX_FIND,
IDS_ESCAPE_CTX_REPLACE,
IDS_ESCAPE_CTX_INSERT,
IDS_ESCAPE_CTX_CLIPBRD,
IDS_ESCAPE_CTX_MACRO,
IDS_ESCAPE_CTX_QUOTE,
IDS_LARGE_PASTE_WARNING,
IDS_ITEMS_REPLACED_ITER,
IDS_ENC_REINTERPRET,
IDS_LFMT_MIXED,
IDS_LFMT_FIXED,
IDS_SETDEF_FORMAT_WARN,
IDS_ENC_BAD,
IDS_ENC_FAILED,
0,
0,
0,
0,
};


LPCTSTR GetStringEx(WORD uID, WORD total, const LPSTR dict, const WORD* dictidx, WORD* dictofs, LPTSTR dictcache, WORD* ofspop, LPCTSTR def){
	WORD i, j, idx;
	LPSTR sp;
	LPTSTR cp;
	for (idx = 0; idx < total; idx++) {
		if (dictidx[idx] == uID)
			break;
	}
	if (idx >= total) return def;
	for (i = *ofspop, j = dictofs[i], sp = dict+j, cp = dictcache+j; i <= idx; i++) {
		for (j=1; *sp; j++)
			*cp++ = *sp++;
		*cp++ = *sp++;
		dictofs[i+1] = dictofs[i] + j;
	}
	*ofspop = MAX(*ofspop, idx+1);
	return dictcache + dictofs[idx];
}

LPCTSTR GetString(WORD uID) {
	static WORD ofs[NUMSTRINGS] = {0}, ofspop = 0;
	static TCHAR strcache[sizeof(strings)];
	static LPTSTR szRsrc = NULL;
	LPTSTR sz = NULL;
	if (hinstThis != hinstLang && uID > IDS_VERSION_SYNCH && (uID < NONLOCALIZED_START || uID > NONLOCALIZED_END) && LoadString(hinstLang, uID, (LPTSTR)&sz, 0)) {
		if (!szRsrc) szRsrc = (LPTSTR)HeapAlloc(globalHeap, 0, MAXSTRING * sizeof(TCHAR));
		LoadString(hinstLang, uID, szRsrc, MAXSTRING);
		return szRsrc;
	}
	return GetStringEx(uID, NUMSTRINGS, (LPSTR)strings, stringsidx, ofs, strcache, &ofspop, _T(""));
}