FROM ubuntu:14.04
MAINTAINER Chris Cannam <cannam@all-day-breakfast.com>
RUN apt-get update && \
    apt-get install -y \
    software-properties-common \
    build-essential \
    libbz2-dev \
    libfftw3-dev \
    libfishsound1-dev \
    libid3tag0-dev \
    liblo-dev \
    liblrdf0-dev \
    libmad0-dev \
    liboggz2-dev \
    libopus-dev \
    libopusfile-dev \
    libpulse-dev \
    libasound2-dev \
    libjack-dev \
    libsamplerate-dev \
    libsndfile-dev \
    libsord-dev \
    libxml2-utils \
    libgl1-mesa-dev \
    raptor-utils \
    librubberband-dev \
    git \
    mercurial \
    curl wget \
    mlton \
    autoconf automake libtool lintian

# NB we do not install portaudio. We don't want to end up including it
# in the bundle, because it comes with a dependency on the JACK
# library which we don't want to bundle and can't assume people will
# have. However, we do install JACK because the Dynamic JACK mechanism
# should ensure we can detect, configure, and use that without
# actually linking against it. We also have Pulse as the default I/O.

RUN apt-add-repository -y ppa:beineri/opt-qt-5.10.1-trusty
RUN apt-get update && \
    apt-get install -y \
    qt510base \
    qt510svg
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en  
ENV LC_ALL en_US.UTF-8

RUN hg clone -r[[REVISION]] https://code.soundsoftware.ac.uk/hg/sonic-visualiser

RUN git clone https://github.com/sandstorm-io/capnproto
WORKDIR capnproto
RUN git checkout v0.6.1
WORKDIR c++
RUN autoreconf -i && ./configure && make -j3 && make install

WORKDIR /root

COPY id_dsa_build .ssh/id_dsa_build
COPY known_hosts .ssh/known_hosts
RUN chmod 600 .ssh/id_dsa_build .ssh/known_hosts
RUN echo '{"accounts": {"bitbucket": "cannam"}}' > .repoint.json
RUN ( echo '[ui]' ; echo 'ssh = ssh -i /root/.ssh/id_dsa_build' ) > .hgrc

WORKDIR /sonic-visualiser
ENV QTDIR /opt/qt510
ENV PATH /opt/qt510/bin:$PATH
RUN ./configure
RUN make -j3

RUN deploy/linux/deploy-appimage.sh
RUN tar cvf output-appimage.tar *.AppImage && cp output-appimage.tar ..
