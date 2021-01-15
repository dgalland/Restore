rem args 1:to_dir 2:from_dir 3:tape or tape_clip 4:fps 5:start 6:Frame count
@echo off

if exist %2\%3 (
	call doclip %1 %2 %3 %4 %5 %6
	exit /b
)

for /f %%f in ('dir /b /ON %2\%3*') do (
	call doclip %1 %2 %%f %4
)

