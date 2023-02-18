FROM ghcr.io/paradise-fi/rofi.debian:latest

SHELL ["/bin/bash", "-c"]

RUN cd /; \
    git clone https://github.com/paradise-fi/RoFI.git --depth 1 --branch bfs-shapes; \
    cd RoFI; \
    source ./setup.sh Release; \
    rcfg desktop; \
    rmake rofi-reconfig; \
    rmake rofi-isoreconfig



# Form an entrypoint that setups environment
RUN echo $'#!/usr/bin/env bash\n\
    source ./setup.sh -s Release;\n\
    $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]
