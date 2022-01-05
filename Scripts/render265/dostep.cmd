rem @echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.265

echo ------------------------------%3 
rem crf lower is better 28 default/
%FFMPEG% -i %input% -c:v libx265 -crf 28 -preset medium -f rawvideo %output% 
rem %X265%   --preset medium  --crf 23. --input-depth 8 --fps 16 --log-level 4 --output %TMP%\%3.265 -f rawvideo --input %INPUT%
