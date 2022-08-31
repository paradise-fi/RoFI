FROM debian:bullseye

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils wget software-properties-common gnupg

RUN env DEBIAN_FRONTEND=noninteractive \
        apt-get update; \
        apt-get install -y --no-install-recommends \
        cmake make git \
        gcc-10 g++-10 \
        libarmadillo-dev libvtk7-dev libvtk7-qt-dev qtdeclarative5-dev \
        gazebo libgazebo-dev libz3-dev

RUN for i in `dpkg-query -L gcc-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L g++-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done

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
    source ./setup.sh Release;\n\
    $CMD $@\n' > /bin/rofiInvoke; \
    chmod +x /bin/rofiInvoke

WORKDIR /RoFI

ENTRYPOINT ["rofiInvoke"]
