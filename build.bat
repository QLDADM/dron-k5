@echo off
rem Buduje firmware RID-K5 w Dockerze. Wynik: scanner.packed.bin
cd /d %~dp0
docker build -t uvk5-toolchain tools || exit /b 1
docker run --rm -v "%~dp0:/src" uvk5-toolchain /bin/bash -c "make clean >/dev/null; make %*"
