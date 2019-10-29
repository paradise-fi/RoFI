FROM debian:buster

RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-utils
RUN env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake make ninja-build clang-7 llvm-7 gcc-8 g++-8 libc++-dev libc++abi-dev \
        clang-tidy-7 valgrind gdb \
        git ssh curl wget rsync python3 python2 ca-certificates acl xmlstarlet \
        apt-file libz3-dev zip unzip locales libarmadillo-dev libvtk6-dev \
        admesh
RUN apt-file update

RUN for i in `apt-file list llvm-7 | cut -d: -f2 | grep '/usr/bin/[^/]*-7'`; do F=`echo $i | sed 's/-7$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `apt-file list clang-7 | cut -d: -f2 | grep '/usr/bin/[^/]*-7'`; do F=`echo $i | sed 's/-7$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `apt-file list clang-tidy-7 | cut -d: -f2 | grep '/usr/bin/[^/]*-7'`; do F=`echo $i | sed 's/-7$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `apt-file list gcc-8 | cut -d: -f2 | grep '/usr/bin/[^/].*-8'`; do F=`echo $i | sed 's/-8$//'`; test -f $F || { echo $F; ln -s $i $F; }; done
RUN for i in `apt-file list g++-8 | cut -d: -f2 | grep '/usr/bin/[^/].*-8'`; do F=`echo $i | sed 's/-8$//'`; test -f $F || { echo $F; ln -s $i $F; }; done

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
    cmake --build build/ --target install

CMD ["bash"]
