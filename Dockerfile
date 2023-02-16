ARG BASE=debian:bullseye

FROM $BASE

# We use bash instead of /bin/sh as ESP-IDF doesn't support /bin/sh
SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils wget software-properties-common gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -; \
    add-apt-repository "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-11 main";

RUN DEBIAN_FRONTEND=noninteractive TZ="Europe/London" \
        apt-get install -y --no-install-recommends \
        build-essential \
        git ssh rsync curl tar \
        clang-11  llvm-11 clang-tidy-11 libc++-11-dev libc++abi-11-dev \
        gcc-10 g++-10 \
        cmake make ninja-build valgrind gdb \
        python3 python3-pip python3-venv python3-dev python3-numpy \
        doxygen graphviz \
        libarmadillo-dev libvtk7-dev libvtk7-qt-dev qtdeclarative5-dev \
        gazebo libgazebo-dev libz3-dev \
        coinor-clp coinor-libclp-dev
RUN pip3 install click

# Install ARM Toolchain for STM32
RUN cd /tmp && \
    wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 && \
    tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 --directory /opt && \
    cd /tmp && \
    rm gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
ENV PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}"
RUN arm-none-eabi-g++ --version
RUN arm-none-eabi-gcc --version

# Install Xtensa Toolchan for ESP32
ENV ROFI_TOOLS_PATH=/opt/esp32
RUN mkdir -p $ROFI_TOOLS_PATH && \
    IDF_PATH=$ROFI_TOOLS_PATH/esp-idf && \
    git clone --depth 1 --branch v4.3.2 --recursive \
            https://github.com/espressif/esp-idf.git $IDF_PATH
RUN export IDF_PATH=$ROFI_TOOLS_PATH/esp-idf && \
    export IDF_TOOLS_PATH=$ROFI_TOOLS_PATH/esp-tools && \
    echo $IDF_PATH, $IDF_TOOLS_PATH && \
    $IDF_PATH/install.sh && \
    # When we use IDF, it brings python venv; thus if we want to build doc,
    # we have to install spinx and breathe into the venv
    . $IDF_PATH/export.sh && \
    pip install sphinx breathe myst_parser sphinx_rtd_theme

# Install Rust
ENV RUSTUP_HOME="/rust"
ENV CARGO_HOME="/cargo"
ENV PATH="/cargo/bin:$PATH"

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain nightly

# Newer Ubuntu (21.10) miss libdl.so which is (probably) required by VTK.
# This is a temporary work-around until we migrate to VTK 9.
RUN if [ ! -e /usr/lib/x86_64-linux-gnu/libdl.so ] ; then \
        ln -s /usr/lib/x86_64-linux-gnu/libdl.so.2 /usr/lib/x86_64-linux-gnu/libdl.so; \
    fi

RUN ldconfig

RUN for i in `dpkg-query -L llvm-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-tidy-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L gcc-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L g++-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done

RUN cmake --version
RUN clang++ --version
RUN g++ --version
RUN cargo --version
RUN rustup show
RUN llvm-cov --version
RUN gcov --version
RUN clang-tidy --version


CMD ["bash"]
