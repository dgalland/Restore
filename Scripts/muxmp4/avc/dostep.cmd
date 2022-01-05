@echo off
rem args 1:to_dir 2:from_dir 3:tape [4:srt_dir]
setlocal enabledelayedexpansion
set input=%2\%3.264
set output=%1\%3.mp4
set srt=%ROOT%\%4\%3.srt
if exist %srt% (
	%MKVMERGE% -o %output% %input%  --chapters %srt%
) else (
	%MKVMERGE% -o %output% %input%
)
