FROM ubuntu:16.04
MAINTAINER Chris Cannam <cannam@all-day-breakfast.com>
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libbz2-dev \
    libfftw3-dev \
    libfishsound1-dev \
    libid3tag0-dev \
    liblo-dev \
    liblrdf0-dev \
    libmad0-dev \
    liboggz2-dev \
    libpulse-dev \
    libsamplerate-dev \
    libsndfile-dev \
    libsord-dev \
    libxml2-utils \
    portaudio19-dev \
    qt5-default libqt5svg5-dev \
    raptor-utils \
    librubberband-dev \
    git \
    mercurial \
    curl wget \
    autoconf automake libtool lintian
RUN apt-get clean && rm -rf /var/lib/apt/lists/*
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en  
ENV LC_ALL en_US.UTF-8
RUN curl -L -o capnproto-v0.6.0.tar.gz https://github.com/sandstorm-io/capnproto/archive/v0.6.0.tar.gz
RUN tar xf capnproto-v0.6.0.tar.gz
WORKDIR capnproto-0.6.0/c++
RUN autoreconf -i 
RUN ./configure --enable-static --disable-shared
RUN make && make install
WORKDIR ../..
RUN hg clone -rsv_v3.0.3 https://code.soundsoftware.ac.uk/hg/sonic-visualiser
WORKDIR sonic-visualiser
RUN ./configure
RUN make -j3
RUN deploy/linux/deploy-deb.sh 3.0.3 amd64
RUN tar cvf output.tar *.deb && cp output.tar ..
