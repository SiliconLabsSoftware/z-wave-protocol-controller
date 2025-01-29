FROM debian:bookworm

ENV VALUE "undefined"

ENV project unifysdk
ENV workdir /usr/local/opt/${project}
ADD . ${workdir}

WORKDIR ${workdir}

RUN echo "# log: Setup system" \
  && set -x  \
  && set \
  && ./helper.mk \
  && date -u

ENTRYPOINT [ "/usr/local/opt/unifysdk/helper.mk" ]
CMD [ "help" ]
