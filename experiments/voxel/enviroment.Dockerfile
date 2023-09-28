FROM ghcr.io/paradise-fi/rofi.debian:latest

SHELL ["/bin/bash", "-c"]

RUN apt-get install -y --no-install-recommends time jq moreutils

RUN cd /; \
    git clone https://github.com/paradise-fi/RoFI.git --depth 1 --branch master; \
    cd RoFI; \
    source ./setup.sh Release; \
    rcfg desktop; \
    rmake rofi-voxel

# Form an entrypoint that setups environment
RUN echo $'#!/usr/bin/env bash\n\
    source ./setup.sh -s Release;\n\
    util/monitorMemUsage.sh /artefact/results.json $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

# Uncomment if you want to enable debug assertions in Rust
# RUN export RUSTFLAGS="-C debug-assertions -C overflow-checks"

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]
