rem Args 1:output_dir 2:input1_dir 3:input2 4:tape
@echo off
set output=%1\%4.avi
set input_1=%2\%4.avi
set input_2=%ROOT%\%3\%4.avi
echo VirtualDub.Open("dostep.avs"^); > dostep.vdscript
type template.vdscript >> "dostep.vdscript"
echo VirtualDub.SaveAVI(U"%output%"^); >> dostep.vdscript

%VDUB64% /i dostep.vdscript /x
