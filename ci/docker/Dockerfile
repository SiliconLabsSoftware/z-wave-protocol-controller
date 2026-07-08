# SPDX-License-Identifier: Zlib
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

FROM debian:trixie AS zpc-dev

USER root

ARG TARGETPLATFORM

RUN if [ "$TARGETPLATFORM" != "linux/amd64" ]; then \
        echo "TARGETPLATFORM: $TARGETPLATFORM"; \
        echo "TARGETPLATFORM is not linux/amd64, unsupported"; \
        exit 1; \
    fi

ENV project=z-wave-protocol-controller

COPY ci/dependencies/apt-packages-base.txt dependencies/apt-packages-base.txt
COPY ci/dependencies/apt-packages-clang-format.txt dependencies/apt-packages-clang-format.txt
COPY ci/dependencies/apt-packages-clang-tidy.txt dependencies/apt-packages-clang-tidy.txt
COPY ci/dependencies/requirements.txt dependencies/requirements.txt

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        $(cat dependencies/apt-packages-base.txt \
               dependencies/apt-packages-clang-format.txt \
               dependencies/apt-packages-clang-tidy.txt)

RUN pip3 install --no-cache-dir -r dependencies/requirements.txt --break-system-packages

WORKDIR /home/zpc

RUN useradd -l -c "Zwave user" -d /home/jenkins -m jenkins

FROM zpc-dev AS zpc-test

USER root
WORKDIR /src

COPY ci/dependencies/apt-runtime-deps.txt dependencies/apt-runtime-deps.txt
COPY . .

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        $(cat dependencies/apt-runtime-deps.txt) \
    && rm -rf /var/lib/apt/lists/* \
    && cmake --preset debian -DCMAKE_INSTALL_PREFIX=/usr \
    && cmake --build build/debian -j "$(nproc)" \
    && cmake --build build/debian --target package \
    && dpkg -i build/debian/zpc-amd64.deb \
    && zpc --help

ENTRYPOINT ["zpc"]
