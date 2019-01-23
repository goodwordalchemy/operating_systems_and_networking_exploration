docker run -it --rm \
    --mount type=bind,src=$(pwd),dst=/workdir \
    -w /workdir \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp=unconfined \
    ostep /bin/bash
