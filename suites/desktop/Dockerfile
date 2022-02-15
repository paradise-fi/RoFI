FROM debian:bullseye

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils wget software-properties-common gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -; \
    add-apt-repository "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-11 main";

RUN env DEBIAN_FRONTEND=noninteractive \
        apt-get update; \
        apt-get install -y --no-install-recommends \
        cmake make ninja-build git ssh rsync valgrind gdb doxygen graphviz python3 python3-pip python3-venv \
        gcc-10 g++-10 \
        clang-11  llvm-11 clang-tidy-11 libc++-11-dev libc++abi-11-dev \
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

RUN ldconfig

CMD ["bash"]
