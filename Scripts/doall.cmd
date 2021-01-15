rem args 1:to_dir 2:from_dir
@echo off
setlocal enabledelayedexpansion

for /f %%f in ('dir /b /ON %ROOT%\%2') do (
    if not exist %ROOT%\%1\%%f (
		set file=%%f
		set file=!file:.= !
		call :sub name !file!
  		call do %1 %2 !name!
	)
)

:sub
set %1=%2
exit/b
