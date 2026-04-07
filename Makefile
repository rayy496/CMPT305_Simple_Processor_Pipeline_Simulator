proj:
	cmake -S cmake -B build
	cmake --build build

clean:
	rm -rf build proj