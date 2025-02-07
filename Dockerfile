FROM debian:bookworm

ENV DEBIAN_FRONTEND noninteractive
ENV LC_ALL en_US.UTF-8
ENV LANG ${LC_ALL}

ARG UNIFYSDK_GIT_REPOSITORY https://github.com/SiliconLabs/UnifySDK
ARG UNIFYSDK_GIT_TAG main

RUN echo "# log: Configuring locales" \
  && set -x  \
  && apt-get update -y \
  && apt-get install -y locales \
  && echo "${LC_ALL} UTF-8" | tee /etc/locale.gen \
  && locale-gen ${LC_ALL} \
  && dpkg-reconfigure locales \
  && TZ=Etc/UTC apt-get -y install tzdata \
  && date -u
  
ENV project z-wave-protocol-controller
ENV workdir /usr/local/opt/${project}
ADD . ${workdir}

WORKDIR ${workdir}

RUN echo "# log: Setup system" \
  && set -x  \
  && apt-get install -y make sudo \
  && ./helper.mk help setup \
  && date -u

RUN echo "# log: Build" \
  && set -x  \
  && ./helper.mk \
  && date -u

ENTRYPOINT [ "/usr/local/opt/z-wave-protocol-controller/helper.mk" ]
CMD [ "help" ]
