rem @echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.mkv

echo ------------------------------%3 PASS 1
%X264%   --preset medium  --crf 28 --output %TMP%\%3.264 %INPUT%
%MKVMERGE% -o %output% %TMP%\%3.264

