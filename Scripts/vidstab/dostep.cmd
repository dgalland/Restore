rem args 1:to_dir 2:from_dir  3:tape
@echo off
set from=%2
echo #Clip list >clips.txt

rem %TOOLS%\ffmpeg\ffmpeg -hide_banner   -i %2\%3.avi -vf vidstabdetect=show=1:result="mytransforms.trf"  -f null -
rem %TOOLS%\ffmpeg\ffmpeg -hide_banner   -i %2\%3.avi -vf vidstabtransform=input="mytransforms.trf"  -codec utvideo %1\%3.avi

%FFMPEG% -hide_banner   -i %2\%3.avi -vf vidstabdetect=shakiness=10:accuracy=15:result="transforms.trf" -f null -
%FFMPEG% -i %2\%3.avi -vf vidstabtransform=smoothing=30:zoom=1:input="transforms.trf"  -codec utvideo %1\%3.avi
