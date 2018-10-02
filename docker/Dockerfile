FROM phusion/baseimage

# System package setup
RUN \ 
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y g++ python3.7 git make && \
    curl https://bootstrap.pypa.io/get-pip.py | python3.7 && \
    pip3.7 install pyyaml cmake

WORKDIR /home

# Get and build isl with coverage info
RUN \
    apt-get install -y automake libtool libgmp-dev && \
    git clone git://repo.or.cz/isl.git && \
    cd isl && ./autogen.sh && \
    CFLAGS="-fprofile-arcs -ftest-coverage" ./configure && \
    make -j && git log --format="%H" -n 1 

# Pull repo
RUN \
    git clone https://github.com/PollyLabs/library-metamorphic-testing.git

# Set workdir to repo location
WORKDIR /home/library-metamorphic-testing

# Setup output folder and link
RUN mkdir /output && ln -s /output ./output

# Copy config file template over and update paths
RUN \
    cp ./config_files/config.yaml.template ./config_files/config_isl.yaml && \
    sed "/^working_dir:/c\working_dir: \"`pwd`/\"" \
        < ./config_files/config.yaml.template \
        > ./config_files/config_isl.yaml && \
    sed -i '/output_tests_folder/s|/out/|/output/|' \
        ./config_files/config_isl.yaml && \
    sed -i '/log_file_path/s|/out/|/output/|' \
        ./config_files/config_isl.yaml

# Update config file path in test_emitter
RUN \
    sed -i "/^const std::string config_file_path/{n;s|.*|\t\"`pwd`/config_files/config_isl.yaml\";|}" ./src/test_emitter.cpp

# Build project
RUN mkdir build && cd build && cmake .. && make -j
RUN cp /home/isl/.libs/libisl* ./libs

# Emit repo version info
RUN \
    git log --format="%H" -n 1 >> /output/metalib_version &&
    git --git-dir /home/isl/.git log --format="%H" -n 1 >> /output/isl_version

# Set command to run experiments
CMD ["python3.7", "./scripts/meta_runner.py", "bounded", "--append-id",\
        "--seed-min", "200", "--seed-max", "210"]

