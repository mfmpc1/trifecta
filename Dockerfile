###############################################################################
# Build this stage for a build environment, e.g.:                             #
#                                                                             #
# docker build --tag mpspdz:buildenv --target buildenv .                      #
#                                                                             #
# The above is equivalent to:                                                 #
#                                                                             #
#   docker build --tag mpspdz:buildenv \                                      #
#     --target buildenv \                                                     #
#     --build-arg arch=native \                                               #
#     --build-arg cxx=clang++-11 \                                            #
#     --build-arg use_ntl=0 \                                                 #
#     --build-arg prep_dir="Player-Data" \                                    #
#     --build-arg ssl_dir="Player-Data"                                       #
#     --build-arg cryptoplayers=0                                             #
#                                                                             #
# To build for an x86-64 architecture, with g++, NTL (for HE), custom         #
# prep_dir & ssl_dir, and to use encrypted channels for 4 players:            #
#                                                                             #
#   docker build --tag mpspdz:buildenv \                                      #
#     --target buildenv \                                                     #
#     --build-arg arch=x86-64 \                                               #
#     --build-arg cxx=g++ \                                                   #
#     --build-arg use_ntl=1 \                                                 #
#     --build-arg prep_dir="/opt/prepdata" \                                  #
#     --build-arg ssl_dir="/opt/ssl"                                          #
#     --build-arg cryptoplayers=4 .                                           #
#                                                                             #
# To work in a container to build different machines, and compile programs:   #
#                                                                             #
# docker run --rm -it mpspdz:buildenv bash                                    #
#                                                                             #
# Once in the container, build a machine and compile a program:               #
#                                                                             #
#   $ make replicated-ring-party.x                                            #
#   $ ./compile.py -R 64 tutorial                                             #
#                                                                             #
###############################################################################
FROM ubuntu:22.04

RUN apt update && apt install -y automake build-essential clang cmake git libboost-dev libboost-thread-dev libgmp-dev libntl-dev libsodium-dev libssl-dev libtool python3 yasm m4

ENV MP_SPDZ_HOME /usr/src/MP-SPDZ
WORKDIR $MP_SPDZ_HOME

# RUN pip install --upgrade pip ipython

COPY . .

ARG arch=native
ARG cxx=clang++-11
ARG use_ntl=0
ARG prep_dir="Player-Data"
ARG ssl_dir="Player-Data"

RUN mkdir -p $prep_dir $ssl_dir

# ssl keys
ARG cryptoplayers=3
ENV PLAYERS ${cryptoplayers}
RUN ./Scripts/setup-ssl.sh ${cryptoplayers} ${ssl_dir}

# RUN make boost libote

RUN ldconfig

RUN cd mpir && ./configure --enable-cxx && make \
        && make check && make install && ldconfig

RUN make multi-replicated-bin-party.x


