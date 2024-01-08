# suggested usage: docker build --rm -t elks-dist:latest . && docker run -it --name elks-dist elks-dist:latest
FROM ubuntu:20.04
ENTRYPOINT ["bash"]
# install required tools, make non-root user and switch to it
ENV USER=builder \
    UID=1000 \
    GID=1000 \
    DEBIAN_FRONTEND=noninteractive \
    DEBCONF_NONINTERACTIVE_SEEN=true
WORKDIR /elks
RUN apt-get update -qq \
 && apt-get install -y --no-install-recommends \
  flex bison texinfo libncurses5-dev \
  bash make g++ git libelf-dev patch \
  xxd ca-certificates wget mtools \
 && rm -r /var/lib/apt/lists /var/cache/apt/archives \
 && addgroup \
    --gid $GID \
    "$USER" \
 && adduser \
    --disabled-password \
    --gecos "" \
    --home "/elks" \
    --ingroup "$USER" \
    --no-create-home \
    --uid "$UID" \
    "$USER" \
 && chown $UID:$GID /elks
USER $USER
# copy in code and build cross tooling
COPY --chown=$USER:$USER . /elks
RUN mkdir -p "cross" \
 && tools/build.sh

# run the rest of the build interactively from step 3: https://github.com/jbruchon/elks/blob/master/BUILD.md
# . ./env.sh
# make menuconfig
# make all

# tarball the results and copy them out of the container
# cd image && tar -cvzf binfiles.tar.gz *.bin

# outside the container, on the host
# docker cp elks-dist:/elks/image/binfiles.tar.gz .
