rem Films directory
#set ROOT=M:\magis
set ROOT=d:\RestoreGit\Films
rem Tools Directory
set TOOLS=d:\RestoreGit\Tools
rem TMP directory
set TMP=d:\tmp

rem Executables
set FFMPEG=%TOOLS%\ffmpeg\bin\ffmpeg
%FFMPEG% -version
set VDUB64=%TOOLS%\virtualdub\vdub64.exe
%VDUB64% /queryVersion
set VDUB32=%TOOLS%\virtualdub\vdub.exe
%VDUB32% /queryVersion
set MKVMERGE=%TOOLS%\mkvtoolnix\mkvmerge
%MKVMERGE% -V
set X264=%TOOLS%\x264\x264.exe
%X264% -h

