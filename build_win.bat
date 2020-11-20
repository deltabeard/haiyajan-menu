@ECHO OFF
SETLOCAL

SET sdlincdir=ext\inc
SET sdllibdir=ext\lib
SET buildtype=DEBUG

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

IF /I %1.==build. (
	IF /I %2.==RELEASE. (SET buildtype=RELEASE)
	IF /I %2.==RELDEBUG. (SET buildtype=RELDEBUG)
	IF /I %2.==RELMINSIZE. (SET buildtype=RELMINSIZE)
	IF /I %2.==DEBUG. (SET buildtype=DEBUG)

	CALL :build
	EXIT /b
)

IF %1.==prepare. (
	CALL :prepare
	EXIT /b
)

IF %1.==help. (
	ECHO USAGE: build_win.bat [build[ DEBUG^|RELEASE^|RELDEBUG^|RELMINSIZE]^|prepare^|help]
	EXIT /b 0
)

REM Prepare and perform full build by default.

:prepare
rem Extract SDL2 headers and pre-compiled libraries
mkdir %sdlincdir%
mkdir %sdllibdir%
expand ext\sdl2_2-0-12_inc.cab -F:* %sdlincdir%
expand ext\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab -F:* %sdllibdir%
if errorlevel 1 (
	echo Could not expand tools\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab
	exit /B
)

:build
set ldlibs=SDL2main.lib SDL2.lib shell32.lib

@ECHO ON
GNUMAKE.EXE -B BUILD=%buildtype% EXTRA_CFLAGS=/I%sdlincdir% EXTRA_LDFLAGS="%ldlibs% /LIBPATH:'%sdllibdir%'"
@ECHO OFF

ENDLOCAL
@ECHO ON
