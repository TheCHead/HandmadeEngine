@echo off
mkdir ..\..\build\handmade
pushd ..\..\build\handmade
cl -Fc -Zi ..\..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd