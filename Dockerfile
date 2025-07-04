# SPDX-License-Identifier: Zlib
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

FROM debian:bookworm AS builder

ARG UNIFYSDK_GIT_REPOSITORY https://github.com/SiliconLabs/UnifySDK
ARG UNIFYSDK_GIT_TAG main

ENV project z-wave-protocol-controller
ENV workdir /usr/local/opt/${project}

ADD . ${workdir}
ARG HELPER="./helper.mk"
ARG HELPER_SETUP_RULES=setup
ARG HELPER_DEFAULT_RULES=default

WORKDIR ${workdir}

RUN echo "# log: Setup system" \
  && set -x  \
  && df -h \
  && apt-get update \
  && apt-get install -y --no-install-recommends -- make sudo \
  && ${HELPER} help ${HELPER_SETUP_RULES} \
  && date -u

RUN echo "# log: Build" \
  && set -x  \
  && ${HELPER} ${HELPER_DEFAULT_RULES} \
  && date -u \
  && echo "# log: Clean to only keep packages to save space" \
  && mkdir -p dist \
  && cd dist \
  && unzip ../build/dist/${project}*.zip \
  && cd - \
  && ${HELPER} distclean \
  && date -u

FROM debian:bookworm
ENV project=z-wave-protocol-controller
ARG workdir=/usr/local/opt/${project}
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
