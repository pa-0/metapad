«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»

                              Metapad 3.7

                    The Ultimate Notepad Replacement

                               Nov 2021

«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»«§»

              (C) Copyright SoBiT Corp,         2021-2024
              (C) Copyright Mario Rugiero,      2013
              (C) Copyright Alexander Davidson, 1999-2011


Contents
--------
- What?
- Why?
- How?
- Distribution
- Installation
- Building
- Changelog
- Contact
- Disclaimer
- License


What?
=====
If you are like me, one of the most useful programs for everyday use is
Microsoft Notepad. I realized that Notepad was quite powerful and did
almost everything I wanted in a simple text editor. Yet I found the UI
to be unlike most 32-bit Windows applications and actually quite poor...

Metapad is a small, fast (and completely free) text editor for all
Windows versions 95 and up with similar features to Microsoft Notepad
but with many extra features and enhancements. It was designed to
completely replace Notepad since it includes all of Notepad's features
and much, much more.


Why?
====
I created this program as a personal project. I was simply annoyed with
the poor usability of Notepad and it was clear that Microsoft wasn't
ever going to improve it (although their latest versions now have some
minor improvements). I was pleased with the outcome of Metapad (I
actually renamed notepad.exe to notepadx.exe and use Metapad instead of
Notepad) so I decided to release it as freeware.


How?
====
Metapad was created (as was Notepad) in pure ANSI C with the Win32 API.
It contained around 1200 lines of code as of version 1 and now has about
9000 lines as of v3.7. As of 2021, it is actually smaller and faster
than Notepad is. Unlike some other so-called Notepad replacements,
Metapad actually loads in an instant. (Those other programs tend to
approach 1 or 2MB instead of being under 200KB and are written using C++
with MFC or even, gasp!, Visual Basic.)


Distribution
============
Metapad is distributed in two flavors: regular and LE (Light Edition)
The regular version uses a RichEdit control whereas LE uses the older
but slightly speedier Edit control. LE may be slightly faster for some
operations but it does not have the following features:
 - Multiple undo/redo
 - Show hyperlinks option
 - Drag & drop text editing
 - Insert/overwrite mode switching

The full release archive contains the following:
metapad/    - Contains the regular metapad executable
metapadLE/  - Contains the LE (Light Edition) metapad executable
ANSI/       - All of the above, but without full Unicode support
                 (these builds are currently recommended for Win9x/Me)
UPX/        - All of the above, but with even smaller filesizes
unicows.dll - Required for running Unicode metapad builds on Win9x/Me
README.txt  - You are reading it
LICENSE.txt - GNU GPL v3


Installation
============
Metapad was designed to completely replace Notepad. To see how you can
install Metapad to replace Notepad see the metaFAQ page on the Metapad
web site (see contact information at the end of this document).

Metapad can also be used as a portable application. In this case,
no installation is required - the executable is ready for action!


Building
========
To build metapad from source, you need Microsoft Visual Studio 2010.
(It may be possible to build with future versions but this is untested.)
Build steps:
 - Install Visual Studio 2010 and its included library and header files
 - Download the latest source code from `github.com/sobitcorp/metapad`
 - Open the solution file `metapad-master.sln`
 - Choose a build configuration and run `Build Solution`
 - The resulting executable file is ready to use on Windows 2000 and up!

For compatibility with Windows 95 and up, the executable file above must
be further hex-edited as there is currently no automation for this.
(This can be done in metapad with `Edit > Replace` ;)
 - Change the PE subsystem version 5.0 -> 4.0:
      `\X001000000002000004000000000000000500000000000000\`  to
      `\X001000000002000004000000000000000400000000000000\`
 - Change a DLL reference:
      `MSVCR100.dll`                                         to
      `MSVCRT.DLL\0\0`

Note that the source also includes the following directories:
 - `old/` - Contains obsolete instructions of historical interest only.
 - `unix/` - Contains makefiles for building with mingw by a previous
      maintainer. These are untested. Feel free to contribute :)


Changelog
=========

(+ indicates bug fix, - indicates new feature)

3.7  Nov 2021
     - full Unicode and UTF-8 support (finally!)
     - lightning-fast Find / Replace All (finally!)
     - wildcards in Find/Replace/Insert dialogs
     - Hex, Decimal, Octal, Binary, and Base-64 escape sequences in 
          Find/Replace/Insert dialogs and Macros
     - Copy/Paste text as Hex and Base-64
     - edit binary files without losing NULLs (Win2k+ only)
     - very long filename support (paths up to 4000 chars, WinXP+ only)
     - separately select Encodings and Line Endings in any combination
     - support for all 100+ codepages in Windows and ability
          to load, save, and convert text between them
     - Insert Text dialog and ability to auto-paste multiple times
     - customizable timestamp insertion (F12 and Ctrl+F12)
     - smart file changed indicator - only when file is actually dirty
     - restore scroll position & selection after Reload/Replace/Undo/etc
     - current selection information in status bar
     - digit grouping option for all displayed numbers
     - user-editable .lng translation plugins (.dlls still supported)
     - full (recursive) Replace All until no search text is remaining     
     - all ANSI C escape sequences in Find/Replace dialogs and Macros
     - 'Close after replace all' option in Replace dialog
     - important keyboard shortcuts work when Find/Replace dialog active
          (use Ctrl+Shift+Z/Y to Undo/Redo in main window)
     - Find/Replace dialogs repositioned to obscure results less
     - prompt when trying to open very large files (perhaps by mistake)
     - go to selection / caret (Ctrl+Alt+Home / Ctrl+Alt+End)
     - clear clipboard
     - transparent mode can be toggled with F11
     - much faster Quote, Un/Tabify, Un/Indent, and Strip functions
     - smaller executable file size through optimizations
     + Find/Replace values no longer trimmed
     + fixed pasting text not correctly inserting newlines
     + fixed URL hitboxes at bottom of file
     + fixed raster font rendering
     + fixed incorrect filesize calculation with some text formats
     + fixed case-insensitive search not working with non-English texts
     + fixed crash when calling Page Setup with no printer installed
     + fixed various problems loading BOM-less UTF-8 and Unicode files
     + fixed position calc. when text contains Unicode Surrogate Pairs
     + minimized window flashing during text updates

3.6  May 2011
     - portability mode: store/load all settings from metapad.ini
     - new high resolution app icon
     - now remembers last open/save folder with option to disable
     - minor UI/usability updates
     - better defaults for general settings
     - UTF-8 file support (provided by David Millis)
     + fixed shift+enter bug (provided by Curtis Bright)
     + fixed convert to title case bug with apostrophes

3.51 + fixed "Can't open clipboard" random message bug
3.5  - support for GUI language plugins (see web site)
     - ten quick buffers (ctrl+alt+numbers to set, alt+numbers to paste)
     - added native Windows XP theme support
     - new Windows XP icon (truecolor with shadow)
     - new Insert File function
     - toolbar button for always on top
     - read only status rechecked on save
     - scroll window horizontally with Alt+Left & Alt+Right
     - sentence case capitalization after any ?, ! and \n
     - no maximum number of favourites (was 16)
     - Shift+Escape for quick save and exit
     - command line option to override plugin version checking
     - command line option to skip loading language plugin
     - opening the settings dialog loads all options from registry
     - can use non-executable files as viewers (e.g., scripts)
     + fixed page at a time mouse scroll wheel bug
     + fixed Chinese Windows copy text bug
     + Shift+Enter adds carriage return
     + fixed crash on loading BOM-less unicode files
     + fixed single character lines changing in WinXP bug
     + fixed minor replace "\n" bug
     + fixed goto line 5 digit limitation
     + upper/lower case changes for intl characters
     + additional minor bug fixes

3.0  - basic Unicode text file support (ANSI code page only)
     - new favourites menu
     - freaky new transparent mode (requires Win2K)
     - new selection margin (see the settings view tab)
     - new block quote function (add "> " to each line)
     - new strip trailing whitespace function
     - new secondary external viewer
     - find & replace now support newlines (\n) and tab characters (\t)
     - sentence case, title case & inverted case functions
     - new launch default (associated) viewer function
     - five new toolbar buttons (word wrap, font, two viewers & refresh)
     - find & replace dialogs use current font option
     - Ctrl+Up and Ctrl+Down scrolls window
     - recent files menu does not dynamically resize
     - now reads Macintosh text files (converts to Unix)
     - separate secondary font colours
     - option to always print with secondary font
     - option to hide the scrollbars when possible (not in LE)
     - option to not select text at cursor for find (uses last find)
     - option to automatically close find dialog
     - can clear find/replace & recent file histories
     - separate option to save find/replace history
     - can specify default file format
     - can suppress "undo buffer" prompt
     - can suppress solitary carriage returns warning
     - replace all in selection will preserve the selection
     - status bar now uses default GUI font (removed status font width)
     - arrow cursor instead of hand for double-click links
     - delete (clear text) on context menu
     - new /e cmdline option to go to end of file
     + Shift+Enter will not lead to invalid files
     + select all menu item updates cut/copy toolbar buttons
     + insert date updates properly over multi-line selections
     + fixed bottom scrollbar disappearing on load
     + paste is now deactivated when no text is on the clipboard
     + initial messageboxes caused no cursor in main window
     + fixed display bug for block indent more than one screen of text
     + recent files list is now case insensitive
     + minor recent files menu display bug fixed
     + fixed .LOG scrolling bug (in LE only)
     + fixed /g command line bug (in LE only)
     + the '&' now appears on printed output (in LE only)

2.0  - no file size limit on Win9X (not in LE)
     - multiple undo/redo (not in LE)
     - show hyperlinks option (not in LE)
     - drag & drop text editing (not in LE)
     - insert/overwrite mode (not in LE)
     - improved printing
     - print selected text
     - custom print margins
     - replace in selection
     - new toolbar (with flat option)
     - find/replace history
     - custom font & background colours
     - always on top mode
     - commit word wrap function
     - unwrap lines function
     - upper & lower case conversion
     - ANSI to OEM text conversion
     - smart home (like DevStudio)
     - launch new instance feature
     - read only toggles file attribute option
     - custom recent files menu size (0 to 16)
     - revamped settings dialog
     + various bug fixes

1.42 + fixed bug concerning long UNC file names
     + fixed wordwrap bug for large binary files
     + fixed minor edit menu bug when Recent on own and Read Only
     + fixed minor status bar redraw bug
     + Replace All was unnecessarily scrolling main window
1.41 - advanced setting controls the status bar width
     + a couple of minor bugs fixed
1.4  - status bar: line, column & total lines; DOS/UNIX/BIN; bytes;
     - Find Next Word (Ctrl+F3, also Ctrl+Shift+F3 for reverse)
     - Strip First Character menu item (for removing '>'s from email)
     - custom file filters in Open and Save As dialogs (see FAQ)
     - goto line offset (in goto dialog)
     - insert date is now on menu (now F6)
     - new long date format menu item (F7)
     - new read only mode (gets set when file is read only)
     - cmdline goto row/col ("/g row:col")
     - no prompt when loading binary files option
     - no time on date insert option
     - no prompt to launch large file in viewer
     - sticky window position feature
     - hide goto offset option (for old behaviour)
     - option to have Recent Files on its own
     - binary files become read only option
     - no save prompt for empty new file
     - files are now opened with full share access
     - faster printing
     - hotkeys for UNIX<->DOS
     - UNIX<->DOS causes dirty file
     + goto line bug fixed (scrolls to cursor)
     + short filename (TESTIN~1.TXT) sendto bug fixed
     + various bugs fixed

1.3  - seamless UNIX text file support
     - block indent and unindent (Tab, Shift+Tab)
     - recent file list
     - "insert tabs as spaces" option
     - refresh current file
     - external printing (/p on cmdline)
     - command line arguments to external viewer
     - separate "save menu settings" option
     - optional smart select double-click

1.2  - Auto-indent mode
     - Go to Line
     - "Hide directory in caption" option
     - "Select current word" (Ctrl+space)
     - doubleclick override (correctly selects word under cursor)
     - reverse searching (Shift+F3)
     - match whole word in Find/Replace
     - optional autosearch from beginning or end (i.e. don't prompt)
     - added quotes to launch filename
     + fixed print dialog bug ("An error occurred...")

1.1  - Added WYSIWYG (optional) printing support. Added more options.
1.0b + Fixed infinite loop when doing Replace All on the same text
1.0a + Fixed Ctrl+A keyboard accelerator
1.0  - Initial release


Contact
=======
When submitting bugs or asking questions please make sure to check this
document and the online FAQ beforehand.

        http://liquidninja.com/metapad/faq.html

The latest releases are currently available on github:

        https://github.com/sobitcorp/metapad/releases
        

Disclaimer
==========
Metapad is freeware and is freely distributable in its unmodified zip
file. Use it at your own risk. The authors and contributors cannot
be held responsible for any problems encountered while using this
software.


License
=======
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
