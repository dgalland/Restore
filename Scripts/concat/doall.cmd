rem args 1:to_dir 2:from_dir 
rem dir de %2 split tape clip voir suan tape change
@echo off
setlocal enabledelayedexpansion
set prev=
for %%f in (%2\*.avi) do (
	set file=%%~nf
	set tc=!file:_= !
	call :sub tape !tc!
	if not !tape! == !prev! (
		set prev=!tape!
		if not exist %1\!tape!.avi (
			call dostep %1 %2 !tape!
		)
	)
)
exit /b

:sub
set %1=%2
exit/b


