@echo off

:query 
set info="y"
set /p info=是否开始删除垃圾文件?(y/n):
if /i "%info%" == "n" (goto end)
if /i "%info%" == "y" (goto begin)
goto query

:begin
del /Q *.sdf
rd /Q /S ipch
del /Q Debug\*.ilk
del /Q Debug\*.pdb
del /Q Release\*.ilk
del /Q Release\*.pdb

rd /Q /S SendVoice\Debug
rd /Q /S SendVoice\Release
rd /Q /S RecieveVoice\Debug
rd /Q /S RecieveVoice\Release

pause
exit

:end
exit