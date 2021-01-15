rem args 1:to 2:from 3:tape_clip 4:fps 5:start 6:frames

rem @echo off
setlocal enabledelayedexpansion

rem echo -------- Joining %1 %2 %3 fps %4 start %5 count %6

rem default without start number
if "%5" == "" (
	set first=%2\%3\image_00001.jpg
	if not exist !first! (
		for /f %%f in ('dir /b /O-N %2\%3\*.jpg') do (
			set file=%%f
		)
	set file=!file:_= !
	set file=!file:.= !
	set num=
	call :sub num !file!
	%FFMPEG% -hide_banner -y -f image2 -framerate %4 -start_number !num! -i %2\%3\image_%%05d.jpg  -codec copy %1\%3.avi
	exit /b
	)
	%FFMPEG% -hide_banner -y -f image2 -framerate %4 -i %2\%3\image_%%05d.jpg -codec copy  %1\%3.avi
	exit /b
)

rem start without count
if "%6" == "" (
	%FFMPEG% -hide_banner -y -f image2 -framerate %4 -start_number %5 -i %2\%3\image_%%05d.jpg  -codec copy %1\%3.avi
	exit /b
)
rem start and count
%FFMPEG% -hide_banner -y -f image2 -framerate %4 -start_number %5 -i %2\%3\image_%%05d.jpg -vframes %6  -codec copy %1\%3.avi
exit /b

:sub
set %1=%3
exit/b


