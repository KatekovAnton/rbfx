container="rokups/crossbuildurho:latest"
docker_params=-it --rm -v $(shell pwd):/workdir -v /tmp/Urho3D-crossbuild:/tmp/Urho3D-crossbuild $(container) ./.crossbuild

cross: cross-linux cross-linux-static cross-osx cross-osx-static cross-win32 cross-win32-static cross-win64 cross-win64-static

prepare:
	mkdir -p /tmp/Urho3D-crossbuild

cross-linux: prepare
	docker run -e CROSS_TRIPLE=linux -e STATIC=1 $(docker_params)

cross-linux-static: prepare
	docker run -e CROSS_TRIPLE=linux -e SHARED=1 $(docker_params)

cross-osx: prepare
	docker run -e CROSS_TRIPLE=osx   -e STATIC=1 $(docker_params)

cross-osx-static: prepare
	docker run -e CROSS_TRIPLE=osx   -e SHARED=1 $(docker_params)

cross-win32: prepare
	docker run -e CROSS_TRIPLE=win32 -e STATIC=1 $(docker_params)

cross-win32-static: prepare
	docker run -e CROSS_TRIPLE=win32 -e SHARED=1 $(docker_params)

cross-win64: prepare
	docker run -e CROSS_TRIPLE=win64 -e STATIC=1 $(docker_params)

cross-win64-static: prepare
	docker run -e CROSS_TRIPLE=win64 -e SHARED=1 $(docker_params)

cross-win: prepare cross-win32 cross-win32-static cross-win64 cross-win64-static

clean:
	rm -rf /tmp/Urho3D-crossbuild

# they way I configure for iOS, cmake 3.19.4, xcode 13.3, mac os 12.4
# the only error - ld cannot find main function, SDL integration is wrong maybe
# hacking it with fake main() fixed the linkage and I can build Samples app
configure_project_ios:
	mkdir -p build && \
	cd build && \
	cmake -G Xcode \
	-T buildsystem=12 \
	-DCMAKE_TOOLCHAIN_FILE=CMake/Toolchains/IOS.cmake \
	-DENABLE_BITCODE=OFF \
	-DPLATFORM=OS64COMBINED \
	-DDEPLOYMENT_TARGET=11.0 \
	-DURHO3D_COMPUTE=OFF \
	-DURHO3D_GRAPHICS_API=GLES2 \
	-DURHO3D_GLOW=OFF \
	-DURHO3D_FEATURES="SYSTEMUI" \
	-DURHO3D_PROFILING=OFF \
	-DURHO3D_PLAYER=OFF \
	-DURHO3D_SAMPLES=ON \
	-DURHO3D_EXTRAS=OFF \
	-DURHO3D_TOOLS=OFF \
	-DURHO3D_RMLUI=ON \
	..
