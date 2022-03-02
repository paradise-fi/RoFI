FROM debian:bullseye

# We use bash instead of /bin/sh as ESP-IDF doesn't support /bin/sh
SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils wget software-properties-common gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -; \
    add-apt-repository "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-11 main";

RUN env DEBIAN_FRONTEND=noninteractive \
        apt-get update; \
        apt-get install -y --no-install-recommends \
        git ssh rsync tar \
        clang-11  llvm-11 clang-tidy-11 libc++-11-dev libc++abi-11-dev \
        gcc-10 g++-10 \
        cmake make ninja-build valgrind gdb \
        python3 python3-pip python3-venv \
        doxygen graphviz \
        libarmadillo-dev libvtk7-dev libvtk7-qt-dev qtdeclarative5-dev \
        gazebo libgazebo-dev libz3-dev

RUN for i in `dpkg-query -L llvm-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-tidy-11 | cut -d: -f2 | grep '/usr/bin/[^/]*-11'`; do F=`echo $i | sed 's/-11//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L gcc-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L g++-10 | cut -d: -f2 | grep '/usr/bin/[^/].*-10'`; do F=`echo $i | sed 's/-10$//'`; test -f $F || { echo $F; ln -s $i $F; }; done

RUN clang++ --version
RUN g++ --version
RUN llvm-cov --version
RUN gcov --version
RUN clang-tidy --version

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
    pip install sphinx breathe recommonmark sphinx_rtd_theme

RUN ldconfig

CMD ["bash"]
