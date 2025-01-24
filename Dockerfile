FROM ubuntu:20.04 AS build
EXPOSE 6025/tcp
EXPOSE 6025/udp

RUN apt-get update && \
         DEBIAN_FRONTEND="noninteractive" \
         apt-get install -y --no-install-recommends \
         build-essential \
         cmake \
         vim \
         wget \
         gcc \
         g++ \
         gdb && \
         apt-get autoremove -y && \
         apt-get clean -y && \
         rm -rf /var/lib/apt/lists/*


RUN mkdir -p /opt/workspace
COPY . /opt/workspace/nethello
WORKDIR /opt/workspace/nethello

RUN mkdir -p build

WORKDIR /opt/workspace/nethello/build
RUN cmake  .. && make -j $(nproc)


FROM ubuntu:20.04

RUN apt-get update && \
         DEBIAN_FRONTEND="noninteractive" \
         apt-get install -y --no-install-recommends \
         build-essential \
         cmake \
         vim \
         wget \
         gcc \
         g++ \
         gdb && \
         apt-get autoremove -y && \
         apt-get clean -y && \
         rm -rf /var/lib/apt/lists/*


RUN mkdir -p /opt/workspace/bin/

WORKDIR /opt/workspace/bin/
COPY --from=build /opt/workspace/nethello/build /opt/workspace/bin/

CMD ["./nethello","-s", "-p", "9090"]