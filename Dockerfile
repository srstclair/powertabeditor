# To use, build the container (make sure you've checked out the git externals):

# docker build -t powertab .

# Install timidity and sounds on the **host** machine:

# apt-get install -y timidity freepats

# Run timidity as a server:

# timidity -iA

# Run the powertab container:

# docker run --rm -it \
#  -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=unix$DISPLAY \
#  --device /dev/snd
#  -v ~/powertab:/root/powertab \
#  --name powertab powertab

# In the powertab window, select File/Preferences and select the timidity midi device for playback

FROM debian:stretch

# Note: do NOT install rapidjson-dev, libpugixml-dev,  catch, or librtmidi-dev.
# Use the git submodules instead (otherwise build will error)

# Also don't install timidity in the container, install it on the host
RUN apt-get update && \
      DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
      git \
      build-essential \
      cmake \
      qtbase5-dev \
      libboost-dev \
      libboost-date-time-dev \
      libboost-filesystem-dev \
      libboost-iostreams-dev \
      libboost-program-options-dev \
      libboost-regex-dev \
      libasound2-dev \
      libiberty-dev \
      binutils-dev \
      zlib1g-dev

RUN mkdir -p /usr/src/powertab

COPY . /usr/src/powertab

WORKDIR /usr/src/powertab

RUN mkdir build \
    && cd build \
    && cmake ..  \
    && make -j8 \
    && make install

CMD powertabeditor
