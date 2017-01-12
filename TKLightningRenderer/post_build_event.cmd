echo off
set configuration=%1

if not exist ..\bin\shader mkdir ..\bin\shader\
if not exist ..\bin\res mkdir ..\bin\res\
copy /Y shader\*.glsl ..\bin\shader\
copy /Y shader\* ..\bin\res\
copy /Y intermediate\%configuration%\*.exe ..\bin\


