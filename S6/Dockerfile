FROM debian:stable-slim
LABEL authors="arnau.sf"
LABEL description="Docker image for the development and testing of the OS Labs."

# Update apt repositories and install Valgrind
RUN apt-get update && apt-get install -y \
    valgrind \
    make \
    gcc
#git \
#gdb

COPY ./src /workspace/src
#COPY ./include /workspace/include
COPY ./Makefile /workspace/

WORKDIR /workspace

RUN echo 'alias val="valgrind -s --read-var-info=yes --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes --track-fds=yes"' >> ~/.bashrc
RUN make clean && make

# Keep the container running (adjust as needed)
CMD ["/bin/bash"]

#ENTRYPOINT ["top", "-b"]
