# syntax=docker/dockerfile:1

ARG DEBIAN_BULLSEYE_DIGEST=sha256:e5b6442dd2e9684cf5e87d8338b5968f3b348636fc0be6d7850a381e3731a2bd

FROM debian@${DEBIAN_BULLSEYE_DIGEST} AS build

ARG PACKAGE
ARG PROJECT_VERSION
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
        DEB|deb) \
            generator="DEB" \
            ;; \
        RPM|rpm) \
            generator="RPM" \
            ;; \
        TGZ|tgz|tar.gz) \
            generator="TGZ" \
            ;; \
        *) \
            echo "Unsupported PACKAGE=${PACKAGE}" >&2 \
            exit 2 \
            ;; \
    esac; \
    case "${TARGETARCH}" in \
        amd64) \
            expected_arch="amd64" \
            ;; \
        arm64) \
            expected_arch="arm64" \
            ;; \
        386) \
            expected_arch="i386" \
            ;; \
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
