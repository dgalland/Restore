@echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.avi
echo VirtualDub.Open(U"%input%"^); > pass1.vdscript
type pass1-template.vdscript >> "pass1.vdscript"
echo VirtualDub.RunNullVideoPass(^);	>> "pass1.vdscript"
echo Pass One
%VDUB64% /i pass1.vdscript m:\tmp\log /x

echo VirtualDub.Open(U"%input%"^); > pass2.vdscript
type pass2-template.vdscript >> pass2.vdscript
echo VirtualDub.SaveAVI(U"%output%"^); >> pass2.vdscript
echo Pass Two
%VDUB64% /i pass2.vdscript /x
