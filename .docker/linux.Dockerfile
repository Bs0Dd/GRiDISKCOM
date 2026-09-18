FROM debian:12-slim AS build

ARG PACKAGE
ARG PROJECT_VERSION=0.0.0
ARG TARGETARCH

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /src

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        dpkg-dev \
        file \
        git \
        ninja-build \
        qtbase5-dev \
        qtbase5-dev-tools \
        rpm \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN set -eux; \
    case "${PACKAGE}" in \
        DEB) generator="DEB" ;; \
        RPM) generator="RPM" ;; \
        TGZ) generator="TGZ" ;; \
        *) \
            echo "Unsupported PACKAGE=${PACKAGE}" >&2 \
            exit 2 \
            ;; \
    esac; \
    case "${TARGETARCH}" in \
        amd64) expected_arch="amd64" ;; \
        arm64) expected_arch="arm64" ;; \
        386) expected_arch="i386" ;; \
        *) \
            echo "Unsupported TARGETARCH=${TARGETARCH}" >&2 \
            exit 2 \
            ;; \
    esac; \
    test "$(dpkg --print-architecture)" = "${expected_arch}"; \
    cmake -S . -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DGRiDISKCOM_VERSION="${PROJECT_VERSION}"; \
    cmake --build build --parallel; \
    rm -rf /out; \
    mkdir -p /out; \
    cpack \
        --config build/CPackConfig.cmake \
        -G "${generator}" \
        -B /out \
        --verbose; \
    test -n "$(find /out -maxdepth 1 -type f -print -quit)"; \
    find /out -maxdepth 1 -type f -print

FROM scratch AS package

COPY --from=build /out/ /out/
