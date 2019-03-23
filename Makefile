all: visualizer
test: visualizer_test

all.clang: visualizer.clang
test.clang: visualizer_test.clang
clean.clang:
	rm -rf _build.clang

all.gcc: visualizer.gcc
test.gcc: visualizer_test.gcc
clean.gcc:
	rm -rf _build.gcc

visualizer.gcc:
	mkdir -p _build.gcc/visualizer
	cd _build.gcc/visualizer; \
	cmake ../../RoFILib/; \
	make

visualizer.clang:
	mkdir -p _build.clang/visualizer
	cd _build.clang/visualizer; \
	CXX=clang++ CC=clang cmake ../../RoFILib/ -DLIBCXX=1; \
	make

visualizer_test.gcc: visualizer.gcc
	cd _build.gcc/visualizer; ./rofi-test

visualizer_test.clang: visualizer.clang
	cd _build.clang/visualizer; ./rofi-test