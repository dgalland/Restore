@echo off
rem do step from args
setlocal 
set step=%1
set output=%ROOT%\%step%
if not exist %output% mkdir %output%
cd %step%
call dostep.cmd %ROOT%\%1 %ROOT%\%2 %3 %4 %5 %6 %7
