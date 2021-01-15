rem args 1:to_dir 2:from_dir
@echo off
for /f %%f in ('dir /b /ON %2') do (
    if not exist %1\%%f.avi (
		call doclip %1 %2 %%f 
	)
)

