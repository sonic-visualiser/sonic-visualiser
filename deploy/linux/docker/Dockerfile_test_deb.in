FROM ubuntu:18.04

# Ensure that everything subsequent is re-run when a new revision is
# being built (rather than being cached) - so as to avoid potential
# mismatches between results of apt-get update and the package
# dependency installation itself
RUN echo [[REVISION]]

RUN apt-get update
ADD output/sonic-visualiser_[[RELEASE]]_amd64.deb sonic-visualiser_[[RELEASE]]_amd64.deb
RUN apt install -y ./sonic-visualiser_[[RELEASE]]_amd64.deb
RUN /usr/bin/sonic-visualiser --version

