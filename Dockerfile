# Docker image with all the dependencies for the prl-disp-service
# Build and run as follows:
# docker build -t prl-disp-service . && docker run -v "$(pwd):/root/src" -it prl-disp-service

FROM virtuozzo/vz7-minimal:latest

RUN printf \
"\n[factory-sources]\n\
name=Build Factory packages for Containers (sources)\n\
baseurl=https://download.openvz.org/virtuozzo/factory/source/SRPMS/\n\
priority=49\n\
enabled=0\n\
gpgcheck=0\n" >>/etc/yum.repos.d/factory.repo

RUN yum -y install yum-utils gdb && \
    yum-config-manager --enable factory factory-sources virtuozzolinux-vz-factory && \
    mkdir /tmp/source_package/ && \
    cd /tmp/source_package/ && \
    yumdownloader --source prl-disp-service && \
    yum -y update && \
    yes | yum-builddep "/tmp/source_package/`ls /tmp/source_package/`"

WORKDIR /root/src

CMD sh docker-build.sh debug && sh docker-build.sh test
