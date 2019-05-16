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
    portaudio19-dev \
    qt5-default libqt5svg5-dev \
    git \
    mercurial \
    curl wget \
    mlton \
    autoconf automake libtool lintian

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
RUN autoreconf -i && ./configure --enable-shared=no --enable-static=yes && make -j3 && make install

WORKDIR /root

COPY id_dsa_build .ssh/id_dsa_build
COPY known_hosts .ssh/known_hosts
RUN chmod 600 .ssh/id_dsa_build .ssh/known_hosts
RUN echo '{"accounts": {"bitbucket": "cannam"}}' > .repoint.json
RUN ( echo '[ui]' ; echo 'ssh = ssh -i /root/.ssh/id_dsa_build' ) > .hgrc

RUN rm -f /usr/lib/x86_64-linux-gnu/librubberband.so*

WORKDIR /sonic-visualiser
RUN ./configure
RUN make -j3

RUN deploy/linux/deploy-deb.sh [[RELEASE]] amd64
RUN tar cvf output-deb.tar *.deb && cp output-deb.tar ..
