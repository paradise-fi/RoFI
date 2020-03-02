FROM debian:buster

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-utils wget software-properties-common gnupg

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -; \
    add-apt-repository "deb http://apt.llvm.org/buster/   llvm-toolchain-buster-9  main"; \
    apt-get update

# Ignore Z3; we will install it later
RUN env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake make ninja-build \
        gcc-8 g++-8 \
        clang-9  llvm-9 clang-tidy-9 libc++-9-dev libc++abi-9-dev libz3-dev- libz3-4- \
        valgrind gdb \
        git ssh rsync python3 python2 ca-certificates acl xmlstarlet \
        zip unzip libarmadillo-dev libvtk6-dev \
        admesh

RUN for i in `dpkg-query -L llvm-9 | cut -d: -f2 | grep '/usr/bin/[^/]*-9'`; do F=`echo $i | sed 's/-9$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-9 | cut -d: -f2 | grep '/usr/bin/[^/]*-9'`; do F=`echo $i | sed 's/-9$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L clang-tidy-9 | cut -d: -f2 | grep '/usr/bin/[^/]*-9'`; do F=`echo $i | sed 's/-9//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L gcc-8 | cut -d: -f2 | grep '/usr/bin/[^/].*-8'`; do F=`echo $i | sed 's/-8$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `dpkg-query -L g++-8 | cut -d: -f2 | grep '/usr/bin/[^/].*-8'`; do F=`echo $i | sed 's/-8$//'`; test -f $F || { echo $F; ln -s $i $F; }; done

RUN clang++ --version
RUN g++ --version
RUN llvm-cov --version
RUN gcov --version
RUN clang-tidy --version

RUN cd /tmp; \
    git clone https://github.com/catchorg/Catch2.git; \
    cd Catch2; \
    git checkout v2.10.2; \
    cmake -Bbuild -H. -DBUILD_TESTING=OFF; \
    cmake --build build/ --target install; \
    cd ..; rm -rf Catch2

RUN cd /tmp; \
    git clone https://github.com/Z3Prover/z3.git; \
    cd z3; \
    git checkout z3-4.8.7; \
    python scripts/mk_make.py; \
    cd build; \
    make -j16; \
    make install; \
    cd ..; rm -rf z3

RUN ldconfig

CMD ["bash"]
