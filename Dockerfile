FROM debian:unstable

RUN apt update -y && apt install build-essential net-tools emacs ethtool curl -y