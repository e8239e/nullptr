build: configure
	cmake --build build

run: build
	./build/nullptr

configure:
	cmake -GNinja -B build -S .

clean:
	rm -rf build

deep-clean: clean
	rm -rf third-party/fonts/*.h \
		third-party/fonts/*.ttf \
		third-party/fonts/*.md \
		third-party/fonts/*.tar* \
		third-party/fonts/*wqy*microhei* \
		third-party/fonts/binary_to_compressed_c \
