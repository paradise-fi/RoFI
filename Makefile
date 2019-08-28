all: rofilib.gcc rofilib.clang
test: rofilib_test

all.clang: rofilib.clang
test.clang: rofilib_test.clang
clean.clang:
	rm -rf _build.clang

all.gcc: rofilib.gcc
test.gcc: rofilib_test.gcc
clean.gcc:
	rm -rf _build.gcc

rofilib.gcc:
	mkdir -p _build.gcc/rofilib
	cd _build.gcc/rofilib; \
	cmake ../../RoFILib/; \
	make

rofilib.clang:
	mkdir -p _build.clang/rofilib
	cd _build.clang/rofilib; \
	CXX=clang++ CC=clang cmake ../../RoFILib/ -DLIBCXX=1; \
	make

rofilib_test.gcc: rofilib.gcc
	cd _build.gcc/rofilib/visualizer; ./rofi-vis-test
	cd _build.gcc/rofilib/reconfig; ./reconfig-test


rofilib_test.clang: rofilib.clang
	cd _build.clang/rofilib/visualizer; ./rofi-vis-test
	cd _build.gcc/rofilib/reconfig; ./reconfig-test
