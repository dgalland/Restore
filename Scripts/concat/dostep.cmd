rem Concatenate clips
rem args 1:to_dir 2:from_dir 3:result  4:clip 6:clip ...
@echo off
setlocal enabledelayedexpansion

set result=%1\%3
set from=%2
echo #Clip list >clips.txt
set /a x=0
shift 
shift
echo !from!

:loop
	@echo %1
	
	for /f %%f in ('dir /b /ON !from!\%1.avi') do (
		set fullname=!from!\%%f
		echo file '!from!\%%f' >> clips.txt
		set /a x+=1
	)
	
	shift
	if not "%~1"=="" goto loop
type clips.txt

%FFMPEG% -hide_banner -y -f concat -safe 0 -i clips.txt -codec copy !result!.avi
exit /b
