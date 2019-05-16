FROM ubuntu:16.04

RUN apt-get -y update && \
    apt-get install -yqq --no-install-recommends \
	software-properties-common autoconf automake autotools-dev g++ pkg-config libtool make && \
    apt-get clean


RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get -y update && \
    apt-get install -yqq --no-install-recommends \
    python2.7-dev python3.5-dev python3.6-dev python3.7-dev && \
    apt-get clean

