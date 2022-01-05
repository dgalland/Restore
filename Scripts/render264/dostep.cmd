rem @echo off
rem args 1:to_dir 2:from_dir tape
set input=%2\%3.avi
set output=%1\%3.264

echo ------------------------------%3 PASS 1
%FFMPEG% -i %input% -c:v libx264 -crf 23 -preset medium -f rawvideo %output% 

