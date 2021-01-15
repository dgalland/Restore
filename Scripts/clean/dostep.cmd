@echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.avi
echo Cleaning %input% %output%
echo VirtualDub.Open("dostep.avs"^); > dostep.vdscript
type template.vdscript >> "dostep.vdscript"
echo VirtualDub.SaveAVI(U"%output%"^); >> dostep.vdscript

%VDUB64% /i dostep.vdscript /x
