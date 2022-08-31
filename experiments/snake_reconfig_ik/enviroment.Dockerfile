FROM gitlab.fi.muni.cz:5050/paradise/mirror/rofi/rofi.debian:latest

SHELL ["/bin/bash", "-c"]

RUN cd /; \
    git clone https://github.com/paradise-fi/RoFI.git; \
    cd RoFI; \
    git checkout newSnakeExperiments; \
    source ./setup.sh Release; \
    rcfg desktop; \
    rmake snakeReconfig

# Form an entrypoint that setups environment
RUN echo $'#!/bin/bash\n\
    CMD=$1;\n\
    shift;\n\
    source ./setup.sh -s Release;\n\
    $CMD $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]
