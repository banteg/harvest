# The Linux desktop the port's smoke test runs in (port/smoke/smoke-linux.sh): Xvfb and a headless
# Weston for X11 and Wayland, Mesa's llvmpipe for OpenGL ES 3.0 and OpenGL 3.3, and PulseAudio,
# PipeWire and ALSA with no sound card. Holds no game files: the build and the data are mounted at
# run time.
FROM debian:trixie-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        xvfb xauth x11-utils weston \
        libgl1 libegl1 libgles2 libgl1-mesa-dri libegl-mesa0 mesa-utils \
        libx11-6 libx11-xcb1 libxext6 libxcursor1 libxi6 libxrandr2 libxfixes3 libxss1 libxtst6 \
        libxkbcommon0 libxkbcommon-x11-0 libwayland-client0 libwayland-cursor0 libwayland-egl1 \
        libdecor-0-0 libdecor-0-plugin-1-cairo \
        pulseaudio pulseaudio-utils pipewire pipewire-pulse wireplumber \
        libasound2t64 libasound2-plugins alsa-utils libpulse0 libdbus-1-3 dbus dbus-user-session libudev1 \
        xclip python3 python3-pil procps tini \
    && rm -rf /var/lib/apt/lists/*

# a user like a desktop one: a home for ~/.Harvest and a runtime directory for the sound servers
RUN useradd --create-home --uid 1000 player \
    && mkdir -p /run/user/1000 && chown player:player /run/user/1000 && chmod 700 /run/user/1000
USER player
ENV HOME=/home/player XDG_RUNTIME_DIR=/run/user/1000
WORKDIR /home/player

ENTRYPOINT ["/usr/bin/tini", "--"]
