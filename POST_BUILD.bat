@echo off
cd "%~dp0libfiledialogs\filedialogs.exe" && echo F | xcopy /y /f "filedialogs.exe" "%~dp0filedialogs.exe"
