FROM debian:unstable

RUN apt update -y && apt install build-essential net-tools emacs ethtool -y

RUN mkdir -p /dev/net
RUN mknod /dev/net/tun c 10 200
RUN chmod 600 /dev/net/tun