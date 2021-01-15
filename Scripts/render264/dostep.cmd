rem @echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.mkv

echo ------------------------------%3 PASS 1
%X264%  --preset medium --pass 1 --bitrate 2000 --stats %TMP%\%3.stats  --output NUL %INPUT%
echo ------------------------------%3 PASS 2
%X264%   --preset medium  --pass 2 --bitrate 2000 --stats  %TMP%\%3.stats --output %TMP%\%3.264 %INPUT%
%MKVMERGE% -o %output% %TMP%\%3.264

