@echo off

cd ..
del /S /Q *.vcproj.*.user
del /S /Q *.ncb
del /S /Q *.aps
del /S /Q *.exp
del /S /Q *.pdb
del /S /Q *.ilk
del /S /Q *.lib
del /S /Q *_d.exe
del /S /Q *_d.dll
rmdir /S /Q src\tmp
