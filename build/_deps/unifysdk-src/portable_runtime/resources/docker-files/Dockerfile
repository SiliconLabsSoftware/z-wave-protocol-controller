FROM debian:bookworm

# Input arguments
ARG DEBIAN_PACKAGES=deb-packages.zip

# Copying over artfiacts
WORKDIR /unify
COPY ${DEBIAN_PACKAGES} .
COPY ./run_zpc.sh .
COPY ./run_matter_bridge.sh .
COPY ./run_eed.sh .
COPY ./uic-mb_1.3.0_amd64.deb .
COPY ./chip-tool .
COPY ./uic-eed_amd64.deb .

RUN chmod +x run_zpc.sh run_matter_bridge.sh run_eed.sh chip-tool

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update \
 && apt install --no-install-recommends -y socat unzip iproute2 \
 && unzip -j *.zip -d deb-packages \
 && echo "deb [trusted=yes] file:///unify/deb-packages ./" > /etc/apt/sources.list.d/uic-packages.list \
 && apt update \
 && apt install -y libreadline-dev libglib2.0-dev \
 && apt install --no-install-recommends -y uic-dev-gui \
                                           uic-gms \
                                           uic-image-provider \
                                           uic-nal \
                                           uic-upti-cap \
                                           uic-upvl \
                                           uic-zpc\
 && apt install -y ./uic-eed_amd64.deb \
 && apt install -y ./uic-mb_1.3.0_amd64.deb \
 && rm -rf /var/lib/apt/lists/* \
 && rm -rf /unify/deb-packages
