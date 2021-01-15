rem args 1:to_dir 2:from_dir
@echo off
for %%f in (%2\*.avi) do (
	if not exist %1\%%~nxf (
		call dostep %1 %2 %%~nf
	)
)

