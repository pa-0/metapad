//Please don't regen this file with automated tools!

/*
ID mapping:
	(*'ed IDs are grandfathered in from v3.6 and cannot be changed without breaking legacy language plugins)
	(s'ed IDs are associated with a string in the strings.c string table)
	(x'ed IDs are nonlocalized)
	(!'ed IDs are conflicting and must not have a string assigned directly to them)
	(starting 3.7+ all IDs, except _BASE/_END range markers and IDC_ dialog control identifiers (1000-1099), MUST be unique - all conflicts have been resolved.)
*s	    1-  111		IDS_ - strings (3.6-, formerly defined in .rc) with some exceptions:
*s	  		    1-  100		various strings (3.6-)
*			  101			IDD_ABOUT - NOT associated with a string
*s	  		  102-  104		various strings (3.6-)
 			  105-  111		 (unused - rsvd)

*s	  112-  128		IDD_ - dialog titles (all associated with a string except 113)
*	  129-  131		IDR_ - menus (NOT associated with a string but must still match up to language plugin rsrc IDs)
	  132-  198		 (unused - rsvd)
 s	  199-  499		IDS_ - strings
 s	  		  199-  249		various strings (3.7+)
			  250-  439		 (unused - rsvd for new strings)
 s			  440-  449		encoding.c strings (3.7+)
 s            450-  499		formerly defined as literals in 3.6-

 sx	  500-  599		IDS_/STR_ nonlocalized strings
 sx			  500-  519		formerly defined as literals in 3.6-
  x	  		  520-  599		(rsvd for nonlocalized strings)

 sx	  600-  699		IDSS_ - setting key names for storing settings in registry or ini file. Used by settings_load/save.c
 s	  700-  799		IDSD_ - setting defaults values (3.7+). Localizable.
	  800-  999		 (unused - rsvd)

*s ! 1000- 1099		IDC_ - common dialog by-command text (fallback if both IDDC_ and IDDP_ matching failed) Contains conflicts.
						Controls in this range should NOT have strings assigned directly to them, unless the string for a particular ID is common to many dialogs.
						Instead, strings should be assigned to the respective IDDC_/IDDP_ ranges. Hence, conflicts in this range are OK.
						Matched by (IDC_ command/control id modulo 100) plus IDC_BASE	(e.g. last 2 digits of the IDC_ id)

	 1100- 1399		 (unused - rsvd)
	 1400- 1499		IDC_ - non-textual or unintialized dialog controls (remapped in 3.7)
*s	 1540- 1599		IDD_ - dialog titles
*s			 1540- 1550		3.6- existing
 s			 1551- 1559		3.7+ new
			 1560- 1599		 (rsvd for dialog titles)

	 1600- 1999		 (unused - rsvd)
*s	 2000- 2999		FC_ - constants for various operational flags which are also associated with a string. Currently used by:
*s						Line format and encoding ids - associated with a string and ID_ menu commands in the 40xxx range

	 3000-19999		 (unused - rsvd)
 s	20000-29999		IDDC_ - per-dialog by-command text (preferred, unless the text has already been manually assigned by program code)
						Matched by ((parent dialog IDD_ id modulo 100) times 100 plus IDDC_BASE) plus (IDC_ command/control id modulo 100)

 s	30000-39999		IDDP_ - per-dialog by-position text (fallback if IDDC_ matching failed)
						Matched by ((parent dialog IDD_ id modulo 100) times 100 plus IDDP_BASE) plus zero-based index position of control modulo 100

*s	40000-40999		ID_ menu commands
*s			40002-40109		3.6- existing (exceptions: some marked ids are currently non-menu commands only but this may change in the future)
 s			40110-40199		3.7 remapped
 s			40200-40299		3.7+ new
			40300-40899		 (rsvd for menu commands)

 s	41000-41999		IDM_ menu by-position text. Currently used for menu items which do not / cannot have IDs assigned.
 						Matched by (parent menu IDR_ id minus IDR_BASE (129)) times 100 plus IDM_BASE (41000) plus position*10 plus submenu-position.
			41000-41099		 (unused - rsvd for IDR_ESCAPE_SEQUENCES)
 s			41100-41199		for IDR_MENU (main menu)
 s			41200-41299		for IDR_POPUP (right-click menu)
			41300-41999		 (rsvd for new menus)

	42000-43999		 (unused - rsvd)
*	44000-44999		ID_ non-menu / accel / manually-inserted commands, with some exceptions:
*			44000-44499		3.6- existing
*s					44200		ID_TRANSPARENT menu command
			44500-44899		 (rsvd for new non-menu commands)
			44900-44999		3.7 remapped

	45000-59999		 (unused - rsvd)
	60000-64999		nonlocalized object IDs
			60000			IDR_ACCELERATOR - not localized starting 3.7+
			61000-61999		IDI_/IDC_/IDB_ graphics - not localized

	65000-65535		 (unused - rsvd)

*/


#define IDC_STATIC (-1)

/* IDS_ strings (3.6-, formerly defined in .rc) */
#define IDS_VERSION_SYNCH				1
#define IDS_PLUGIN_LANGUAGE				2
#define IDS_PLUGIN_RELEASE				3
#define IDS_PLUGIN_TRANSLATOR			4
#define IDS_PLUGIN_EMAIL				5
//#define IDS_RESERVED2					6
//#define IDS_RESERVED3					7
#define IDS_DEFAULT_FILTER				8
#define IDS_DIRTYFILE					9
#define IDS_ERROR_LOCKED				10
#define IDS_ERROR_SEARCH				11
#define IDS_LAUNCH_WARNING				12
#define IDS_READONLY_INDICATOR			13
//#define IDS_READONLY_MENU				14
#define IDS_ERROR_FAVOURITES			15
#define IDS_FILE_LOADING				16
#define IDS_READONLY_WARNING			17
#define IDS_PRINTER_NOT_FOUND			18
#define IDS_PRINT_INIT_ERROR			19
#define IDS_PRINT_ABORT_ERROR			20
#define IDS_PRINT_START_ERROR			21
#define IDS_DRAWTEXT_ERROR				22
#define IDS_PRINT_ERROR					23
#define IDS_VIEWER_ERROR				24
//#define IDS_SECONDARY_VIEWER_ERROR	25
#define IDS_VIEWER_MISSING				26
//#define IDS_SECONDARY_VIEWER_MISSING	27
#define IDS_NO_DEFAULT_VIEWER			28
//#define IDS_DEFAULT_VIEWER_ERROR		29
#define IDS_REGISTRY_WINDOW_ERROR		30
//#define IDS_CARRIAGE_RETURN_WARNING	31
//#define IDS_MAC_FILE_WARNING			32
#define IDS_CREATE_FILE_MESSAGE			33
#define IDS_FILE_CREATE_ERROR			34
#define IDS_FILE_NOT_FOUND				35
#define IDS_FILE_LOCKED_ERROR			36
#define IDS_UNICODE_CONVERT_ERROR		37
#define IDS_UNICODE_CHARS_WARNING		38
#define IDS_BINARY_FILE_WARNING			39
#define IDS_UNICODE_STRING_ERROR		40
#define IDS_WRITE_BOM_ERROR				41
#define IDS_FONT_UNDO_WARNING			42
#define IDS_CHAR_WIDTH_ERROR			43
#define IDS_PARA_FORMAT_ERROR			44
#define IDS_QUERY_SEARCH_TOP			45
#define IDS_QUERY_SEARCH_BOTTOM			46
#define IDS_RECENT_MENU					47
//#define IDS_RECENT_FILES_MENU			48
#define IDS_MAX_RECENT_WARNING			49
#define IDS_STICKY_MESSAGE				50
#define IDS_CLEAR_FIND_WARNING			51
#define IDS_CLEAR_RECENT_WARNING		52
#define IDS_SELECT_PLUGIN_WARNING		53
#define IDS_MARGIN_WIDTH_WARNING		55
#define IDS_TRANSPARENCY_WARNING		56
#define IDS_TAB_SIZE_WARNING			57
#define IDS_TB_NEWFILE					59
#define IDS_TB_OPENFILE					60
#define IDS_TB_SAVEFILE					61
#define IDS_TB_PRINT					62
#define IDS_TB_FIND						63
#define IDS_TB_REPLACE					64
#define IDS_TB_CUT						65
#define IDS_TB_COPY						66
#define IDS_TB_PASTE					67
#define IDS_TB_UNDO						68
#define IDS_TB_REDO						69
#define IDS_TB_SETTINGS					70
#define IDS_TB_REFRESH					71
#define IDS_TB_WORDWRAP					72
#define IDS_TB_PRIMARYFONT				73
#define IDS_TB_ONTOP					74
#define IDS_TB_PRIMARYVIEWER			75
#define IDS_TB_SECONDARYVIEWER			76
#define IDS_CANT_UNDO_WARNING			77
#define IDS_MEMORY_LIMIT				78
#define IDS_QUERY_LAUNCH_VIEWER			79
#define IDS_LE_MEMORY_LIMIT				80
#define IDS_UNDO_HYPERLINKS_WARNING		81
#define IDS_CHANGE_READONLY_ERROR		82
#define IDS_SETTINGS_TITLE				83
#define IDS_NO_SELECTED_TEXT			84
//#define IDS_BYTE_LENGTH				85
#define IDS_ITEMS_REPLACED				86
#define IDS_MENU_LANGUAGE_PLUGIN		87
#define IDS_RICHED_MISSING_ERROR		88
#define IDS_COMMAND_LINE_OPTIONS		89
#define IDS_GLOBALLOCK_ERROR			90
#define IDS_CLIPBOARD_UNLOCK_ERROR		91
#define IDS_CLIPBOARD_OPEN_ERROR		92
#define IDS_FILE_READ_ERROR				93
//#define IDS_RESTART_HIDE_SB			94
//#define IDS_RESTART_FAVES				95
//#define IDS_RESTART_LANG				96
#define IDS_INVALID_PLUGIN_ERROR		97
#define IDS_BAD_STRING_PLUGIN_ERROR		98
#define IDS_PLUGIN_MISMATCH_ERROR		99
#define IDS_ALLRIGHTS					100
#define IDD_ABOUT						101		//not associated with a string
#define IDS_DEFAULT_FILTER_TEXT			102
#define IDS_NEW_FILE					103
#define IDS_MACRO_LENGTH_ERROR			104
//#define IDS_NEW_INSTANCE				105

/* IDD_ dialog titles */
#define IDD_PROPPAGE_GENERAL			112
#define IDD_ABORT_PRINT					113		//not associated with a string
#define IDD_GOTO						114
#define IDD_PROPPAGE_VIEW				124
#define IDD_FAV_NAME					128

/* IDR_ menus (NOT associated with a string but must still match up to language plugin rsrc IDs) */
#define IDR_BASE						129		//(range)
#define IDR_ESCAPE_SEQUENCES			129
#define IDR_MENU						130
#define IDR_POPUP						131

/* IDS_ strings 3.7+ */
#define IDS_ERROR						199
#define IDS_ERROR_MSG					200
#define IDS_UNICODE_SAVE_TRUNCATION		201
#define IDS_UNICODE_LOAD_ERROR			202
#define IDS_UNICODE_LOAD_TRUNCATION		203
#define IDS_BINARY_FILE_WARNING_SAFE	204
#define IDS_LARGE_FILE_WARNING			205
#define IDS_UNICODE_CONV_ERROR			206
#define IDS_ESCAPE_ERROR				207
#define IDS_ESCAPE_BADCHARS				208
#define IDS_ESCAPE_BADALIGN				209
#define IDS_ESCAPE_EXPECTED				210
#define IDS_ESCAPE_CTX_FIND				211
#define IDS_ESCAPE_CTX_REPLACE			212
#define IDS_ESCAPE_CTX_INSERT			213
#define IDS_ESCAPE_CTX_CLIPBRD			214
#define IDS_ESCAPE_CTX_MACRO			215
#define IDS_ESCAPE_CTX_QUOTE			216
#define IDS_LARGE_PASTE_WARNING			217
#define IDS_ITEMS_REPLACED_ITER			218
#define IDS_ENC_REINTERPRET				219
#define IDS_LFMT_MIXED					220
#define IDS_LFMT_FIXED					221
#define IDS_SETDEF_FORMAT_WARN			222
#define IDS_ENC_BAD						223
#define IDS_ENC_FAILED					224
#define IDS_FILE_SAVING					225

#define IDS_DECODEBASE_BADLEN			441
#define IDS_DECODEBASE_BADCHAR			442
#define IDS_MIGRATED					490
#define IDS_STATFMT_BYTES				491
#define IDS_STATFMT_LINES				492
#define IDS_STATFMT_SEL					493
#define IDS_STATFMT_LINE				494
#define IDS_STATFMT_COL					495
#define IDS_STATFMT_INS					498
#define IDS_STATFMT_OVR					499

/* IDS_/STR_ nonlocalized strings */
#define NONLOCALIZED_BASE				500		//(range)
#define NONLOCALIZED_END				699		//(range)
#define STR_METAPAD						500
#define STR_CAPTION_FILE				501
#define STR_ABOUT						502
#define STR_FAV_FILE					506
#define STR_INI_FILE					507
#define STR_URL							508
#define STR_REGKEY						509
#define STR_FAV_APPNAME					510
#define STR_OPTIONS						511
#define STR_COPYRIGHT					512
#define IDS_PLUGIN_ERRFIND				513
#define IDS_PLUGIN_ERR					514
#define IDS_FILTER_EXEC					518
#define IDS_FILTER_PLUGIN				519
#define IDS_LOADLNG_ERROR				520

/* IDSS_ setting key names for storing settings in registry or ini file */
#define IDSS_WSTATE						600
#define IDSS_WLEFT						601
#define IDSS_WTOP						602
#define IDSS_WWIDTH						603
#define IDSS_WHEIGHT					604
#define IDSS_MRU						605
#define IDSS_MRUTOP						606
#define IDSS_HIDEGOTOOFFSET				607
#define IDSS_SYSTEMCOLOURS				608
#define IDSS_SYSTEMCOLOURS2				609
#define IDSS_NOSMARTHOME				610
#define IDSS_NOAUTOSAVEEXT				611
#define IDSS_CONTEXTCURSOR				612
#define IDSS_CURRENTFINDFONT			613
#define IDSS_PRINTWITHSECONDARYFONT		614
#define IDSS_NOSAVEHISTORY				615
#define IDSS_NOFINDAUTOSELECT			616
#define IDSS_RECENTONOWN				617
#define IDSS_DONTINSERTTIME				618
#define IDSS_NOWARNINGPROMPT			619
#define IDSS_UNFLATTOOLBAR				620
#define IDSS_STICKYWINDOW				621
#define IDSS_READONLYMENU				622
#define IDSS_SELECTIONMARGINWIDTH		623
#define IDSS_MAXMRU						624
#define IDSS_FORMAT						625
#define IDSS_TRANSPARENTPCT				626
#define IDSS_NOCAPTIONDIR				627
#define IDSS_AUTOINDENT					628
#define IDSS_INSERTSPACES				629
#define IDSS_FINDAUTOWRAP				630
#define IDSS_QUICKEXIT					631
#define IDSS_SAVEWINDOWPLACEMENT		632
#define IDSS_SAVEMENUSETTINGS			633
#define IDSS_SAVEDIRECTORY				634
#define IDSS_LAUNCHCLOSE				635
#define IDSS_NOFAVES					636
#define IDSS_DEFAULTPRINTFONT			637
#define IDSS_ALWAYSLAUNCH				638
#define IDSS_LINKDOUBLECLICK			639
#define IDSS_HIDESCROLLBARS				640
#define IDSS_SUPPRESSUNDOBUFFERPROMPT	641
#define IDSS_LAUNCHSAVE					642
#define IDSS_TABSTOPS					643
#define IDSS_NPRIMARYFONT				644
#define IDSS_NSECONDARYFONT				645
#define IDSS_PRIMARYFONT				648
#define IDSS_SECONDARYFONT				649
#define IDSS_BROWSER					650
#define IDSS_BROWSER2					651
#define IDSS_LANGPLUGIN					652
#define IDSS_FAVDIR						653
#define IDSS_ARGS						654
#define IDSS_ARGS2						655
#define IDSS_QUOTE						656
#define IDSS_CUSTOMDATE					657
#define IDSS_CUSTOMDATE2				658
#define IDSS_MACROARRAY					659
#define IDSS_BACKCOLOUR					660
#define IDSS_FONTCOLOUR					661
#define IDSS_BACKCOLOUR2				662
#define IDSS_FONTCOLOUR2				663
#define IDSS_MARGINS					664
#define IDSS_WORDWRAP					665
#define IDSS_FONTIDX					666
#define IDSS_SMARTSELECT				667
#define IDSS_HYPERLINKS					668
#define IDSS_SHOWSTATUS					669
#define IDSS_SHOWTOOLBAR				670
#define IDSS_ALWAYSONTOP				671
#define IDSS_TRANSPARENT				672
#define IDSS_CLOSEAFTERFIND				673
#define IDSS_CLOSEAFTERREPLACE			674
#define IDSS_CLOSEAFTERINSERT			675
#define IDSS_NOFINDHIDDEN				676
#define IDSS_FILEFILTER					677
#define IDSS_FINDARRAY					678
#define IDSS_REPLACEARRAY				679
#define IDSS_INSERTARRAY				680
#define IDSS_LASTDIRECTORY				681

/* IDSD_ setting defaults values (3.7+). Localizable. */
#define IDSD_QUOTE						756
#define IDSD_CUSTOMDATE					757

/* IDC_ common dialog by-command text (strings common to more than one dialog, assigned directly to IDC_ ids). */
#define IDC_BASE						1000	//(range)
#define IDC_OK							1001	//IDOK = 1
#define IDC_CANCEL						1002	//IDCANCEL = 2
//#define IDC_CLOSE						1008	//IDCLOSE = 8
#define IDC_BUTTON_BROWSE				1013
#define IDC_BUTTON_BROWSE2				1015
#define ID_SMARTSELECT					1037	//ID_ menu command (40xxx)! Not moveable due to legacy
#define IDC_FIND_MATCH_WORD				1040	//Win32 constants for std find dialog
#define IDC_FIND_MATCH_CASE				1041	//Win32 constants for std find dialog
#define IDC_OFFSET_TEXT					1043
#define IDC_RADIO_SELECTION				1045
#define IDC_RADIO_WHOLE					1046
#define IDC_CLOSE_AFTER_FIND			1076
#define IDC_NAME						1078
#define IDC_RADIO_LANG_DEFAULT			1086	//3.7 remapped - was conflicting and doesn't need to be localized
#define IDC_RADIO_LANG_PLUGIN			1087	//3.7 remapped - was conflicting and doesn't need to be localized
#define IDC_TEXT_MACRO					1088
#define IDC_CLOSE_AFTER_INSERT			1089
#define IDC_CLOSE_AFTER_REPLACE			1090
#define IDC_TEXT_FIND					1091
#define IDC_TEXT_REPLACE				1092

/* IDC_ unique dialog by-command text (strings for these are assigned to respective IDDC_ ranges, rather than directly to the IDC_ ids, to avoid any conflicts) */
#define IDC_CHECK_QUICKEXIT				1010	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_CHECK_SAVEWINDOWPLACEMENT   1011	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_CHECK_LAUNCH_CLOSE			1016	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_LAUNCH_SAVE0				1030	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_LAUNCH_SAVE1				1031	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_LAUNCH_SAVE2				1032	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_FIND_AUTO_WRAP				1033	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_AUTO_INDENT					1034	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_INSERT_SPACES				1035	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_NO_CAPTION_DIR				1036	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_CHECK_SAVEMENUSETTINGS		1038	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_CHECK_SAVEDIRECTORY			1041	//IDDC_PROPPAGE_GENERAL_BASE
#define IDC_FONT_PRIMARY				1018	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_FONT_SECONDARY				1019	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_FONT_PRIMARY_2				1020	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_FONT_SECONDARY_2			1021	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_BTN_FONT1					1023	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_BTN_FONT2					1024	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_SYSTEM_COLOURS				1050	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_SYSTEM_COLOURS2				1053	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_STAT_WIND					1056	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_STAT_FONT					1057	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_STAT_WIND2					1058	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_STAT_FONT2					1059	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_FLAT_TOOLBAR				1066	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_STAT_TRANS					1081	//IDDC_PROPPAGE_VIEW_BASE
#define IDC_GOTO_HIDE_OFFSET			1036	//IDDC_PROPPAGE_A1_BASE
#define IDC_NUM_MRU						1041	//IDDC_PROPPAGE_A1_BASE
#define IDC_RECENT						1042	//IDDC_PROPPAGE_A1_BASE
#define IDC_INSERT_TIME					1043	//IDDC_PROPPAGE_A1_BASE
#define IDC_DEFAULT_PRINT				1045	//IDDC_PROPPAGE_A1_BASE
#define IDC_PROMPT_BINARY				1044	//IDDC_PROPPAGE_A1_BASE
#define IDC_CURRENT_FIND_FONT			1046	//IDDC_PROPPAGE_A1_BASE
#define IDC_STICKY_WINDOW				1047	//IDDC_PROPPAGE_A1_BASE
#define IDC_BUTTON_STICK				1048	//IDDC_PROPPAGE_A1_BASE
#define IDC_SECONDARY_PRINT_FONT		1049	//IDDC_PROPPAGE_A1_BASE
#define IDC_SMARTHOME					1059	//IDDC_PROPPAGE_A1_BASE
#define IDC_READONLY_MENU				1060	//IDDC_PROPPAGE_A1_BASE
#define IDC_ALWAYS_LAUNCH				1067	//IDDC_PROPPAGE_A1_BASE
#define IDC_LINK_DC						1068	//IDDC_PROPPAGE_A1_BASE
#define IDC_NO_SAVE_EXTENSIONS			1069	//IDDC_PROPPAGE_A1_BASE
#define IDC_CONTEXT_CURSOR				1070	//IDDC_PROPPAGE_A1_BASE
#define IDC_SUPPRESS_UNDO_PROMPT		1071	//IDDC_PROPPAGE_A1_BASE
#define IDC_BUTTON_CLEAR_FIND			1072	//IDDC_PROPPAGE_A1_BASE
#define IDC_BUTTON_CLEAR_RECENT			1073	//IDDC_PROPPAGE_A1_BASE
#define IDC_BUTTON_FORMAT				1074	//IDDC_PROPPAGE_A1_BASE
#define IDC_HIDE_SCROLLBARS				1075	//IDDC_PROPPAGE_A1_BASE
#define IDC_NO_SAVE_HISTORY				1076	//IDDC_PROPPAGE_A1_BASE
#define IDC_NO_FAVES					1077	//IDDC_PROPPAGE_A1_BASE
#define IDC_NO_FIND_SELECT				1078	//IDDC_PROPPAGE_A1_BASE

/* IDC_ non-textual or unintialized dialog controls (remapped in 3.7) */
#define IDC_DLGICON						1400	//3.7: moved from 1009. non-textual
#define IDC_NUM							1401	//3.7: moved from 1088. unintialized
#define IDC_DATA						1402	//3.7: moved from 1077. unintialized
#define IDC_STATICX						1405	//3.7: moved from 1005. unintialized
#define IDC_EDIT_URL					1407	//3.7: moved from 1007. unintialized
#define IDC_EDIT_PLUGIN_RELEASE			1408	//3.7: moved from 1008. unintialized
#define IDC_EDIT_PLUGIN_EMAIL			1409	//3.7: moved from 1009. unintialized
#define IDC_EDIT_PLUGIN_TRANSLATOR		1410	//3.7: moved from 1010. unintialized
#define IDC_EDIT_PLUGIN_LANG			1411	//3.7: moved from 1083. unintialized
#define IDC_EDIT_BROWSER				1412	//3.7: moved from 1012. unintialized
#define IDC_EDIT_BROWSER2				1414	//3.7: moved from 1014. unintialized
#define IDC_TAB_STOP					1425	//3.7: moved from 1025. unintialized
#define IDC_LINE						1435	//3.7: moved from 1035. unintialized
#define IDC_EDIT_ARGS					1439	//3.7: moved from 1039. unintialized
#define IDC_EDIT_ARGS2					1440	//3.7: moved from 1040. unintialized
#define IDC_OFFSET						1444	//3.7: moved from 1044. unintialized
#define IDC_EDIT_MAX_MRU				1450	//3.7: moved from 1050. unintialized
#define IDC_MACRO_1						1461	//3.7: moved from 1070. unintialized
//(1461-1470 rsvd for IDC_MACRO_x)		1470
#define IDC_EDIT_SM_WIDTH				1471	//3.7: moved from 1071. unintialized
#define IDC_EDIT_TRANSPARENT			1472	//3.7: moved from 1072. unintialized
#define IDC_EDIT_QUOTE					1479	//3.7: moved from 1079. unintialized
#define IDC_ESCAPE						1080	//3.7: moved from 1080. non-textual
#define IDC_ESCAPE2						1081	//3.7: moved from 1081. non-textual
#define IDC_EDIT_LANG_PLUGIN			1482	//3.7: moved from 1082. unintialized
#define IDC_STATIC_COPYRIGHT			1484	//3.7: moved from 1084. unintialized
#define IDC_STATIC_COPYRIGHT2			1485	//3.7: moved from 1085. unintialized
#define IDC_CUSTOMDATE					1486	//3.7: moved from 1086. unintialized
#define IDC_CUSTOMDATE2					1487	//3.7: moved from 1087. unintialized
#define IDC_DROP_FIND					1491	//3.7: moved from 1154. unintialized
#define IDC_DROP_REPLACE				1492	//3.7: moved from 1153. unintialized
#define IDC_DROP_INSERT					1493
#define IDC_COLOUR_BACK					1496	//3.7: moved from 1048. non-textual
#define IDC_COLOUR_FONT					1497	//3.7: moved from 1049. non-textual
#define IDC_COLOUR_BACK2				1498	//3.7: moved from 1051. non-textual
#define IDC_COLOUR_FONT2				1499	//3.7: moved from 1052. non-textual

/* IDD_ dialog titles */
#define IDD_FIND						1540
#define IDD_REPLACE						1541
#define IDD_PAGE_SETUP					1546
#define IDD_PROPPAGE_A1					1547
#define IDD_PROPPAGE_A1_LE				1548
#define IDD_PROPPAGE_A2					1549
#define IDD_ABOUT_PLUGIN				1550
#define IDD_INSERT						1551
#define IDD_CP							1552

/* FC_ constants for various operational flags which are also associated with a string */
#define FC_BASE							2000	//7d0	(range)
#define FC_LFMT_BASE					2039	//7f7	(range)
#define FC_LFMT_END						2051	//803	(range)
#define FC_ENC_BASE						2052	//804	(range)
#define FC_ENC_END						2119	//847	(range)
#define FC_ENC_UNKNOWN					2052	//804	corresponds to ID_ENC_UNKNOWN	(40052)
#define FC_ENC_ANSI						2111	//83f	corresponds to ID_ENC_ANSI		(40111)
#define FC_ENC_UTF8						2109	//83d	corresponds to ID_ENC_UTF8		(40109), grandfathered in from 3.6-
#define FC_ENC_UTF16					2089	//829	corresponds to ID_ENC_UTF16		(40089), grandfathered in from 3.6-
#define FC_ENC_UTF16BE					2090	//82a	corresponds to ID_ENC_UTF16BE	(40090), grandfathered in from 3.6-
#define FC_ENC_BIN						2112	//840	corresponds to ID_ENC_BIN		(40112)
#define FC_ENC_CODEPAGE					2118	//846	corresponds to ID_ENC_CODEPAGE	(40118)
//#define FC_ENC_CUSTOM					2119	//847	corresponds to ID_ENC_CUSTOM	(40119)
#define FC_LFMT_UNKNOWN					2051	//803	corresponds to ID_LFMT_UNKNOWN	(40051)
#define FC_LFMT_DOS						2039	//7f7	corresponds to ID_LFMT_DOS		(40039), grandfathered in from 3.6-
#define FC_LFMT_UNIX					2040	//7f8	corresponds to ID_LFMT_UNIX		(40040), grandfathered in from 3.6-
#define FC_LFMT_MAC						2048	//800	corresponds to ID_LFMT_MAC		(40048)
#define FC_LFMT_MIXED					2049	//801	corresponds to ID_LFMT_MIXED	(40049)

/* IDDC_ per-dialog by-command text (preferred, unless the text has already been manually assigned by program code) */
#define IDDC_BASE						20000	//(range)
#define IDDC_PROPPAGE_GENERAL_BASE		21200	//(range)
#define IDDC_PROPPAGE_VIEW_BASE			22400	//(range)
#define IDDC_FIND_BASE					24000	//(range)
#define IDDC_REPLACE_BASE				24100	//(range)
#define IDDC_PAGE_SETUP_BASE			24600	//(range)
#define IDDC_PROPPAGE_A1_BASE			24700	//(range)
#define IDDC_INSERT_BASE				25100	//(range)

/* IDDP_ per-dialog by-position text (fallback if IDDC_ matching failed) */
#define IDDP_BASE						30000	//(range)
#define IDDP_PROPPAGE_GENERAL_BASE		31200	//(range)
#define IDDP_ABORT_PRINT_BASE			31300	//(range)
#define IDDP_GOTO_BASE					31400	//(range)
#define IDDP_PROPPAGE_VIEW_BASE			32400	//(range)
#define IDDP_REPLACE_BASE				34100	//(range)
#define IDDP_PROPPAGE_A2_BASE			34900	//(range)
#define IDDP_ABOUT_PLUGIN_BASE			35000	//(range)
#define IDDP_INSERT_BASE				35100	//(range)

/* ID_ menu commands */
#define ID_MENUCMD_BASE					40000	//(range)
#define ID_HELP_ABOUT					40002
#define ID_MYFILE_OPEN					40003
#define ID_MYFILE_EXIT					40004
#define ID_MYFILE_SAVEAS				40009
#define ID_EDIT_WORDWRAP				40011
#define ID_MYFILE_NEW					40012
#define ID_MYFILE_SAVE					40013
#define ID_FILE_LAUNCHVIEWER			40017
#define ID_VIEW_OPTIONS					40020
#define ID_DATE_TIME					40021
#define ID_FONT_PRIMARY					40023
#define ID_FIND							40026
#define ID_REPLACE						40027
#define ID_FIND_NEXT					40028
#define ID_MYEDIT_UNDO					40029
#define ID_MYEDIT_CUT					40030
#define ID_MYEDIT_COPY					40031
#define ID_MYEDIT_PASTE					40032
#define ID_MYEDIT_SELECTALL				40033
#define ID_PRINT						40035
#define ID_EDIT_SELECTWORD				40036
#define ID_GOTOLINE						40037
#define ID_FIND_PREV					40038
#define ID_LFMT_DOS						40039
#define ID_LFMT_UNIX					40040
#define ID_INDENT						40041
#define ID_UNINDENT						40043
#define ID_LFMT_MAC						40048	//3.7+
#define ID_LFMT_MIXED					40049	//3.7+
#define ID_LFMT_UNKNOWN					40051	//3.7+
#define ID_ENC_UNKNOWN					40052	//3.7+
#define ID_LFMT_BASE					40039	//(range)
#define ID_LFMT_END						40051	//(range)
#define ID_ENC_BASE						40052	//(range)
#define ID_ENC_END						40119	//(range)
#define ID_RELOAD_CURRENT				40053
#define ID_MYEDIT_REDO					40054
#define ID_READONLY						40055
#define ID_SHOWSTATUS					40057
#define ID_DATE_TIME_LONG				40058
#define ID_FIND_NEXT_WORD				40059
#define ID_FIND_PREV_WORD				44060	//3.7: moved from 102. not associated with a string. used by accelerator only
#define ID_STRIPCHAR					40062
#define ID_MAKE_OEM						40063
#define ID_MAKE_ANSI					40064
#define ID_COMMIT_WORDWRAP				40065
//#define ID_SHOWFILESIZE					40066
#define ID_SHOWHYPERLINKS				40067
//#define IDD_MRU_1						40068
#define ID_PAGESETUP					40071
#define ID_MAKE_UPPER					40072
#define ID_MAKE_LOWER					40073
#define ID_STRIP_CR						40074
#define ID_SHOWTOOLBAR					40075
#define ID_TABIFY						40078
#define ID_UNTABIFY						40079
#define ID_HACKER						40080	//not associated with a string. used by accelerator only
#define ID_NEW_INSTANCE					40081
#define ID_ALWAYSONTOP					40082
#define ID_STRIP_CR_SPACE				40084
#define ID_LAUNCH_ASSOCIATED_VIEWER		40087
#define ID_STRIP_TRAILING_WS			40088
#define ID_ENC_UTF16					40089
#define ID_ENC_UTF16BE					40090
#define ID_MYEDIT_DELETE				40091
#define ID_FAV_ADD						40092
#define ID_FAV_EDIT						40093
#define ID_FAV_RELOAD					40094
#define ID_QUOTE						40095
#define ID_MAKE_INVERSE					40096
#define ID_MAKE_SENTENCE				40097
#define ID_MAKE_TITLE					40098
#define ID_ESCAPE_NEWLINE				40099
#define ID_INSERT_FILE					40100
#define ID_ESCAPE_BACKSLASH				40101
#define ID_ESCAPE_DISABLE				40102
#define ID_LAUNCH_SECONDARY_VIEWER		40105
#define ID_ENC_UTF8						40109
#define ID_ESCAPE_TAB					40110	//3.7 moved from 40100 due to conflict
#define ID_ENC_ANSI						40111	//3.7+
#define ID_ENC_BIN						40112	//3.7+
#define ID_ENC_CODEPAGE					40118	//3.7+
#define ID_ENC_CUSTOM					40119	//3.7+

#define ID_DATE_TIME_CUSTOM				40204
#define ID_DATE_TIME_CUSTOM2			40205
#define ID_PASTE_MUL					40206
#define ID_INSERT_TEXT					40207
#define ID_COPY_HEX						40208
#define ID_PASTE_HEX					40209
#define ID_CLEAR_CLIPBRD				40210
#define ID_ESCAPE_HEX					40211
#define ID_ESCAPE_DEC					40212
#define ID_ESCAPE_OCT					40213
#define ID_ESCAPE_BIN					40214
#define ID_ESCAPE_HEXU					40215
#define ID_ESCAPE_HEXS					40216
#define ID_ESCAPE_HEXSU					40217
#define ID_ESCAPE_B64S					40218
#define ID_ESCAPE_B64SU					40219
#define ID_COPY_B64						40220
#define ID_PASTE_B64					40221
//#define ID_BASECONV					40222
#define ID_ESCAPE_ANY					40223
#define ID_ESCAPE_RAND					40224
#define ID_ESCAPE_WILD0					40225
#define ID_ESCAPE_WILD1					40226
#define ID_ESCAPE_REP0					40227
#define ID_ESCAPE_REP1					40228

/* IDM_ menu by-position text. Currently used for menu items which do not / cannot have IDs assigned. */
#define IDM_BASE						41000	//(range)
#define IDM_MENU_BASE					41100	//(range)
#define IDM_POPUP_BASE					41200	//(range)

/* ID_ non-menu / accel / manually-inserted commands */
#define ID_MRU_BASE						44000
#define ID_MRU_1						44001
#define ID_MRU_2						44002
#define ID_MRU_3						44003
#define ID_MRU_4						44004
#define ID_MRU_5						44005
#define ID_MRU_6						44006
#define ID_MRU_7						44007
#define ID_MRU_8						44008
#define ID_MRU_9						44009
#define ID_MRU_10						44010
#define ID_MRU_11						44011
#define ID_MRU_12						44012
#define ID_MRU_13						44013
#define ID_MRU_14						44014
#define ID_MRU_15						44015
#define ID_MRU_16						44016
#define ID_FAV_RANGE_BASE				44100
#define ID_FAV_RANGE_MAX				44199
#define ID_TRANSPARENT					44200	//ID_ menu command (40xxx)! Not moveable due to legacy
#define ID_MACRO_1						44301
#define ID_MACRO_2						44302
#define ID_MACRO_3						44303
#define ID_MACRO_4						44304
#define ID_MACRO_5						44305
#define ID_MACRO_6						44306
#define ID_MACRO_7						44307
#define ID_MACRO_8						44308
#define ID_MACRO_9						44309
#define ID_MACRO_10						44310
#define ID_SET_MACRO_1					44400
#define ID_SET_MACRO_2					44401
#define ID_SET_MACRO_3					44402
#define ID_SET_MACRO_4					44403
#define ID_SET_MACRO_5					44404
#define ID_SET_MACRO_6					44405
#define ID_SET_MACRO_7					44406
#define ID_SET_MACRO_8					44407
#define ID_SET_MACRO_9					44408
#define ID_SET_MACRO_10					44409
#define ID_SHIFT_ENTER					44904	//3.7: moved from 40104. not associated with a string. used by accelerator only
#define ID_SCROLLLEFT					44906	//3.7: moved from 40106. not associated with a string. used by accelerator only
#define ID_SCROLLRIGHT					44907	//3.7: moved from 40107. not associated with a string. used by accelerator only
#define ID_SAVE_AND_QUIT				44908	//3.7: moved from 40108. not associated with a string. used by accelerator only
#define ID_CONTROL_SHIFT_ENTER			44910	//3.7: moved from 40110. not associated with a string. used by accelerator only
#define ID_MYFILE_QUICK_EXIT			44914	//3.7: moved from 40014. not associated with a string. used by accelerator only
#define ID_ABOUT_PLUGIN					44954	//3.7: moved from 104. not associated with a string. used as ID for function inserted into main menu dynamically
#define ID_INSERT_MODE					44961	//3.7: moved from 40061. not associated with a string. used as ID for function inserted into main menu dynamically
#define ID_HOME							44970	//3.7: moved from 40070. not associated with a string. used as ID for function inserted into main menu dynamically
#define ID_CONTEXTMENU					44983	//3.7: moved from 40083. not associated with a string. used as ID for function inserted into main menu dynamically
#define ID_SCROLLUP						44985	//3.7: moved from 40085. not associated with a string. used as ID for function inserted into main menu dynamically
#define ID_SCROLLDOWN					44986	//3.7: moved from 40086. not associated with a string. used as ID for function inserted into main menu dynamically

/* nonlocalized object IDs */
#define IDR_ACCELERATOR					60000	//3.7: moved from 104. Starting 3.7+ accelerators are not localized!
#define IDI_PAD							61109	//3.7: moved from 109. Graphics are not localized
#define IDI_EYE							61110	//3.7: moved from 110. Graphics are not localized
#define IDC_MYHAND						61122	//3.7: moved from 122. Graphics are not localized
#define IDB_TOOLBAR						61126	//3.7: moved from 126. Graphics are not localized
#define IDB_DROP_ARROW					61131	//3.7: moved from 131. Graphics are not localized



// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC						1
#define _APS_NEXT_RESOURCE_VALUE		50000
#define _APS_NEXT_COMMAND_VALUE			50000
#define _APS_NEXT_CONTROL_VALUE			50000
#define _APS_NEXT_SYMED_VALUE			50000
#endif
#endif
