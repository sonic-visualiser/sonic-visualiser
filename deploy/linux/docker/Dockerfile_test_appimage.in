FROM centos:7

# Ensure that everything subsequent is re-run when a new revision is
# being built (rather than being cached) - so as to avoid potential
# mismatches between results of yum update and the package dependency
# installation itself
RUN echo [[REVISION]]

RUN yum -y update
RUN yum -y groupinstall "X Window System"
RUN yum -y install wget
ADD output/SonicVisualiser-[[REVISION]]-x86_64.AppImage SV.AppImage
RUN chmod +x SV.AppImage
RUN ./SV.AppImage --appimage-extract
RUN ./squashfs-root/AppRun --version
