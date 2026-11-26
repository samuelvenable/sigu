@echo off
cd "%~dp0out\build\*-*\.cache\catalog\src_*\src" && echo D | xcopy /y /f /s /e "include\SDL3" "%~dp0libfiledialogs\source\SDL3"
cd "%~dp0out\build\*-*\.cache" && echo D | xcopy /y /f /s /e "..\.cache" "%~dp0libfiledialogs\.cache"
cd "%~dp0out\build\*-*\.cache\catalog\build_*" && echo F | xcopy /y /f "SDL3-*.lib" "%~dp0SDL3.lib"
