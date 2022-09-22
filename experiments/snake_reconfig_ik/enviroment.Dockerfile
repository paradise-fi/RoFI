FROM gitlab.fi.muni.cz:5050/paradise/mirror/rofi/rofi.debian:latest

SHELL ["/bin/bash", "-c"]

RUN cd /; \
    git clone https://github.com/paradise-fi/RoFI.git; \
    cd RoFI; \
    git checkout newSnakeExperiments; \
    source ./setup.sh Release; \
    rcfg desktop; \
    rmake rofi-freconfig

# Form an entrypoint that setups environment
RUN echo $'#!/bin/bash\n\
    CMD=$1;\n\
    shift;\n\
    source ./setup.sh -s Release;\n\
    util/monitorMemUsage.sh /artefact/results.json $CMD $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]
