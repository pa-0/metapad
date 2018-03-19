# Makefile for MSVC

EXE=metapad.exe
OBJ=cdecode.obj cencode.obj metapad.obj
RES=metapad.res

$(EXE): $(OBJ) $(RES)
    link /nologo /out:$(EXE) $(OBJ) $(RES) comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib

.rc.res: 
	rc /nologo $<

.c.obj:
	cl /nologo /c /MT /DNO_RICH_EDIT /DNDEBUG /D_MBCS /DWIN32 /D_WINDOWS $<
	
clean:
	del /f *.obj *.res *.exe