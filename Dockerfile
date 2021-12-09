ARG DOCKER_IMAGE=alpine:3.14
FROM $DOCKER_IMAGE AS builder

ENV metaworld_GAME_VERSION master
ENV IRRLICHT_VERSION master

COPY .git /usr/src/metaworld/.git
COPY CMakeLists.txt /usr/src/metaworld/CMakeLists.txt
COPY README.md /usr/src/metaworld/README.md
COPY metaworld.conf.example /usr/src/metaworld/metaworld.conf.example
COPY builtin /usr/src/metaworld/builtin
COPY cmake /usr/src/metaworld/cmake
COPY doc /usr/src/metaworld/doc
COPY fonts /usr/src/metaworld/fonts
COPY lib /usr/src/metaworld/lib
COPY misc /usr/src/metaworld/misc
COPY po /usr/src/metaworld/po
COPY src /usr/src/metaworld/src
COPY textures /usr/src/metaworld/textures

WORKDIR /usr/src/metaworld

RUN apk add --no-cache git build-base cmake sqlite-dev curl-dev zlib-dev zstd-dev \
		gmp-dev jsoncpp-dev postgresql-dev ninja luajit-dev ca-certificates && \
	git clone --depth=1 -b ${metaworld_GAME_VERSION} https://github.com/metaworld/metaworld_game.git ./games/metaworld_game && \
	rm -fr ./games/metaworld_game/.git

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp/ && \
	mkdir prometheus-cpp/build && \
	cd prometheus-cpp/build && \
	cmake .. \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_TESTING=0 \
		-GNinja && \
	ninja && \
	ninja install

RUN git clone --depth=1 https://github.com/metaworld/irrlicht/ -b ${IRRLICHT_VERSION} && \
	cp -r irrlicht/include /usr/include/irrlichtmt

WORKDIR /usr/src/metaworld
RUN mkdir build && \
	cd build && \
	cmake .. \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DENABLE_PROMETHEUS=TRUE \
		-DBUILD_UNITTESTS=FALSE \
		-DBUILD_CLIENT=FALSE \
		-GNinja && \
	ninja && \
	ninja install

ARG DOCKER_IMAGE=alpine:3.14
FROM $DOCKER_IMAGE AS runtime

RUN apk add --no-cache sqlite-libs curl gmp libstdc++ libgcc libpq luajit jsoncpp zstd-libs && \
	adduser -D metaworld --uid 30000 -h /var/lib/metaworld && \
	chown -R metaworld:metaworld /var/lib/metaworld

WORKDIR /var/lib/metaworld

COPY --from=builder /usr/local/share/metaworld /usr/local/share/metaworld
COPY --from=builder /usr/local/bin/metaworldserver /usr/local/bin/metaworldserver
COPY --from=builder /usr/local/share/doc/metaworld/metaworld.conf.example /etc/metaworld/metaworld.conf

USER metaworld:metaworld

EXPOSE 30000/udp 30000/tcp

CMD ["/usr/local/bin/metaworldserver", "--config", "/etc/metaworld/metaworld.conf"]
