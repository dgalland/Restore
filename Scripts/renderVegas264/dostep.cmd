@echo off
rem args 1:to_dir 2:from_dir 3:tape
#vegas output
set input=%2\%3.avi
set srt=%2\%3.srt
set output=%1\%3.mkv


rem echo ------------------------------%3 PASS 1
rem %X264%  --preset medium --pass 1 --bitrate 2000 --stats %TMP%\%3.stats  --output NUL %INPUT%
rem echo ------------------------------%3 PASS 2
rem %X264%   --preset medium  --pass 2 --bitrate 2000 --stats  %TMP%\%3.stats --output %TMP%\%3.264 %INPUT%
%X264%   --preset medium  --crf 28 --output %TMP%\%3.264 %INPUT%
if exist %srt% (
	%MKVMERGE% -o %output% %TMP%\%3.264  --chapters %srt%
) else (
	%MKVMERGE% -o %output% %TMP%\%3.264
)
