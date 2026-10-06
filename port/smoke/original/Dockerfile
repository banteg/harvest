# A Linux desktop for the original 1.18 Linux build (x86-64 only): its system libraries (NVIDIA's
# Cg from non-free, GTK 2, OpenAL, ALUT, libjpeg 6.2, GLU), Xvfb with Mesa, xdotool to drive it, and
# PulseAudio with a null sink. port/smoke/original/run.sh plays it to make user data for the port.
# Holds no game files: the game is mounted at run time.
FROM debian:trixie-slim

RUN sed -i 's/^Components: main.*/Components: main contrib non-free/' /etc/apt/sources.list.d/debian.sources \
    && apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        xvfb xauth x11-utils xdotool \
        libgl1 libgl1-mesa-dri libglu1-mesa libcg libcggl \
        libgtk2.0-0t64 libopenal1 libalut0 libvorbisfile3 libjpeg62-turbo zlib1g libxrandr2 \
        pulseaudio pulseaudio-utils python3 python3-pil procps tini \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --uid 1000 player \
    && mkdir -p /run/user/1000 && chown player:player /run/user/1000 && chmod 700 /run/user/1000
USER player
ENV HOME=/home/player XDG_RUNTIME_DIR=/run/user/1000
WORKDIR /home/player

ENTRYPOINT ["/usr/bin/tini", "--"]
