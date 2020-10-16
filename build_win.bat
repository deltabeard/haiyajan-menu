@ECHO OFF
SETLOCAL

IF NOT DEFINED VSCMD_VER (
	ECHO You must run this within Visual Studio Native Tools Command Line
	EXIT /B 1
)

WHERE GNUMAKE.EXE >nul 2>nul
IF ERRORLEVEL 1 (
	ECHO GNUMAKE.EXE was not found
	REM You can compile GNU Make within Visual Studio Native Tools Command
	REM Line natively by executing build_w32.bat within the root folder of
	REM GNU Make source code.
	REM You can download GNU Make source code at
	REM   http://ftpmirror.gnu.org/make/
	EXIT /B 1
)

set sdlincdir=ext\inc
set sdllibdir=ext\lib

rem Extract SDL2 headers and pre-compiled libraries
mkdir %sdlincdir%
mkdir %sdllibdir%
expand ext\sdl2_2-0-12_inc.cab -F:* %sdlincdir%
expand ext\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab -F:* %sdllibdir%
if errorlevel 1 (
	echo Could not expand tools\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab 
	exit /B
)

@ECHO ON
GNUMAKE.EXE EXTRA_CFLAGS=/I%sdlincdir% EXTRA_LDFLAGS="SDL2main.lib SDL2-static.lib winmm.lib msimg32.lib version.lib imm32.lib setupapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /LIBPATH:'%sdllibdir%'"
@ECHO OFF

ENDLOCAL
@ECHO ON
