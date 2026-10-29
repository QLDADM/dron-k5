@echo off
rem Pobiera egzumer (commit 7607f0a), dodaje tryb DRON DETEKTOR i buduje w Dockerze.
rem Wynik: egzumer\firmware.packed.bin
cd /d %~dp0
if not exist egzumer git clone https://github.com/egzumer/uv-k5-firmware-custom.git egzumer || exit /b 1
cd egzumer
git checkout -q 7607f0a || exit /b 1
git checkout -q -- .
git apply ..\egzumer-dron-detektor.patch || exit /b 1
copy /y ..\dronedet.c app\ >nul
copy /y ..\dronedet.h app\ >nul
cd ..
docker build -t uvk5-toolchain ..\tools || exit /b 1
docker run --rm -v "%~dp0egzumer:/src" uvk5-toolchain /bin/bash -c "make clean >/dev/null; make %*"
