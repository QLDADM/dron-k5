# Toolchain do budowy firmware UV-K5 (egzumer + detektor dronów).
# Źródła są montowane jako wolumin, obraz zawiera tylko narzędzia.
FROM ubuntu:24.04
RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      make gcc-arm-none-eabi libnewlib-arm-none-eabi python3 python3-crcmod git ca-certificates \
 && ln -s /usr/bin/python3 /usr/local/bin/python \
 && git config --global --add safe.directory '*' \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
