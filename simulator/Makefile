
default: build-all

GAZEBO_ARGS ?= "--verbose"

EXAMPLE ?= basic_joint_force
EXAMPLE_ARGS ?=

.PHONY: clean build-all run tests-roficom example-server example-client

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


example-server: build-all
	./runGazebo.sh ./examples/${EXAMPLE}/${EXAMPLE}.world ${GAZEBO_ARGS}

example-client: build-all
	./build/examples/${EXAMPLE}/${EXAMPLE} ${EXAMPLE_ARGS}
