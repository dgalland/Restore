rem args 1:to_dir 2:from_dir
@echo off
setlocal enabledelayedexpansion

for /f %%f in ('dir /b /ON %ROOT%\%2') do (
	set file=%%~nf
	echo !file!
	set count=0
	for /f %%x in ('dir /b /ON %ROOT%\%1\!file!.*') do set /a count+=1
	if !count! == 0 (
		call do %1 %2 !file! %3 %4 %5
	)
)

