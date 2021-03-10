.PHONY: clean

debug:
	@mkdir -p build.debug
	@cd build.debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j8 --no-print-directory

release:
	@mkdir -p build.release
	@cd build.release && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 --no-print-directory

logic:
	@mkdir -p build.logic
	@cd build.logic && cmake -DCMAKE_BUILD_TYPE=Debug ../test/logic && make -j8 --no-print-directory
	@build.logic/test

native:
	@mkdir -p build.native
	@cd build.native && cmake -DCMAKE_BUILD_TYPE=Debug ../test/native && make -j8 --no-print-directory
	@cd build.native && make --no-print-directory flash

flash: flash_debug
flash_debug: debug
	@cd build.debug && make --no-print-directory flash
flash_release: release
	@cd build.release && make --no-print-directory flash

clean:
	@rm -rf build.debug build.release build.native build.logic
