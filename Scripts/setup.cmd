rem Films directory
rem set ROOT=M:\magis
set ROOT=d:\Restore\Films
rem Tools Directory
set TOOLS=d:\Restore\Tools
rem TMP directory
set TMP=d:\tmp

rem Executables
set FFMPEG=%TOOLS%\ffmpeg\bin\ffmpeg
set FFPROBE=%TOOLS%\ffmpeg\bin\ffprobe
%FFMPEG% -version
set VDUB64=%TOOLS%\virtualdub\vdub64.exe
%VDUB64% /queryVersion
set VDUB32=%TOOLS%\virtualdub\vdub.exe
%VDUB32% /queryVersion
set MKVMERGE=%TOOLS%\mkvtoolnix\mkvmerge
%MKVMERGE% -V
set X264=%TOOLS%\x264\x264.exe
%X264% -h
set X265=%TOOLS%\x265\x265-8b.exe
%X265% -VDUB32

