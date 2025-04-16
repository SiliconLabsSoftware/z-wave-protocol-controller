# SPDX-License-Identifier: Zlib
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

FROM debian:bookworm as builder

ARG UNIFYSDK_GIT_REPOSITORY https://github.com/SiliconLabs/UnifySDK
ARG UNIFYSDK_GIT_TAG main

ENV project z-wave-protocol-controller
ENV workdir /usr/local/opt/${project}
ADD . ${workdir}

WORKDIR ${workdir}

RUN echo "# log: Setup system" \
  && set -x  \
  && df -h \
  && apt-get update \
  && apt-get install -y --no-install-recommends -- make sudo \
  && ./helper.mk help setup \
  && date -u

RUN echo "# log: Build" \
  && set -x  \
  && ./helper.mk \
  && date -u \
  && echo "# log: Clean to only keep packages to save space" \
  && mkdir -p dist \
  && cd dist \
  && unzip ../build/dist/${project}*.zip \
  && cd - \
  && ./helper.mk distclean \
  && date -u

FROM debian:bookworm
ENV project z-wave-protocol-controller
ENV workdir /usr/local/opt/${project}
COPY --from=builder ${workdir}/dist/ ${workdir}/dist/
WORKDIR ${workdir}

RUN echo "# log: Install to system" \
  && set -x  \
  && apt-get update \
  && dpkg -i ./dist/${project}*/*.deb \
  || apt install -f -y --no-install-recommends \
  && echo "TODO: rm -rf dist # If artifacts are no more needed" \
  && apt-get clean -y \
  && rm -rf /var/lib/{apt,dpkg,cache,log}/ \
  && df -h \
  && date -u

ENTRYPOINT [ "/usr/bin/zpc" ]
CMD [ "--help" ]
