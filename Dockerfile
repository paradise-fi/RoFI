ARG BASE=debian:bookworm

FROM $BASE

# We use bash instead of /bin/sh as ESP-IDF doesn't support /bin/sh
SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils ca-certificates curl git gnupg libusb-1.0-0 python3-click rsync software-properties-common ssh tar wget

RUN DEBIAN_FRONTEND=noninteractive TZ="Europe/London" \
        apt-get install -y --no-install-recommends \
        build-essential \
        clang llvm clang-tidy libc++-dev libc++abi-dev \
        gcc g++ \
        cmake make ninja-build valgrind gdb \
        python3 python3-pip python3-venv python3-dev python3-numpy \
        doxygen graphviz \
        libarmadillo-dev libvtk9-dev libvtk9-qt-dev qtdeclarative5-dev \
        libz3-dev \
        coinor-clp coinor-libclp-dev

# Install Gazebo Sim packages on Ubuntu Noble images. The Debian image keeps a
# leaner headless toolchain and is still used for non-simulator lanes.
RUN . /etc/os-release && \
    if [ "$ID" = "ubuntu" ]; then \
        curl -fsSL https://packages.osrfoundation.org/gazebo.gpg \
            -o /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg && \
        echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] https://packages.osrfoundation.org/gazebo/ubuntu-stable ${VERSION_CODENAME} main" \
            > /etc/apt/sources.list.d/gazebo-stable.list && \
        apt-get update && \
        DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
            gz-sim10-cli \
            gz-sim10-server \
            libgz-common7-dev \
            libgz-gui10-dev \
            libgz-math9-dev \
            libgz-msgs12-dev \
            libgz-plugin4-dev \
            libgz-sim10-dev \
            libgz-transport15-dev; \
    fi

# Install ARM Toolchain for STM32
RUN cd /tmp && \
    curl -fsSL -o arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz \
        https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz && \
    tar -xf arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz --directory /opt && \
    cd /tmp && \
    rm arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz
ENV PATH="/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin:${PATH}"
RUN arm-none-eabi-g++ --version
RUN arm-none-eabi-gcc --version

# Install Xtensa Toolchan for ESP32
ENV ROFI_TOOLS_PATH=/opt/esp32
ENV ROFI_ESP_IDF_VERSION=v6.0
RUN mkdir -p $ROFI_TOOLS_PATH && \
    IDF_PATH=$ROFI_TOOLS_PATH/esp-idf && \
    git clone --depth 1 --branch ${ROFI_ESP_IDF_VERSION} --recursive \
            https://github.com/espressif/esp-idf.git $IDF_PATH
RUN export IDF_PATH=$ROFI_TOOLS_PATH/esp-idf && \
    export IDF_TOOLS_PATH=$ROFI_TOOLS_PATH/esp-tools && \
    echo $IDF_PATH, $IDF_TOOLS_PATH && \
    $IDF_PATH/install.sh esp32 && \
    # When we use IDF, it brings python venv; thus if we want to build doc,
    # we have to install spinx and breathe into the venv
    . $IDF_PATH/export.sh && \
    pip install sphinx breathe myst_parser sphinx_rtd_theme

# Install Rust
ENV RUSTUP_HOME="/rust"
ENV CARGO_HOME="/cargo"
ENV PATH="/cargo/bin:${PATH}"

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain 1.95.0

# Some Noble images omit the unversioned libdl soname that older VTK / Qt
# tooling still looks for.
RUN if [ ! -e /usr/lib/x86_64-linux-gnu/libdl.so ] ; then \
        ln -s /usr/lib/x86_64-linux-gnu/libdl.so.2 /usr/lib/x86_64-linux-gnu/libdl.so; \
    fi

RUN ldconfig

RUN cmake --version
RUN clang++ --version
RUN g++ --version
RUN cargo --version
RUN rustup show
RUN llvm-cov --version
RUN gcov --version
RUN clang-tidy --version


CMD ["bash"]
