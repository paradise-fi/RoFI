
default: build-all
.PHONY: clean build-all

GAZEBO_ARGS ?= "--verbose"

EXAMPLE ?= basic_joint_force
EXAMPLE_ARGS ?= ""


build:
	mkdir -p build
	cd build && cmake ..

build-all: build
	cd build && make -j8

clean:
	rm -rf build

run: build-all
	./runGazebo.sh ${GAZEBO_ARGS}

tests-roficom: build-all
	./build/plugins/roficom/tests/tests_roficom


example-client: build-all
	./build/examples/${EXAMPLE}/${EXAMPLE} ${EXAMPLE_ARGS}
