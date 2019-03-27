[![Build Status](https://travis-ci.org/PollyLabs/library-metamorphic-testing.svg?branch=master)](https://travis-ci.org/PollyLabs/library-metamorphic-testing)

C++ infrastructure for automated metamorphic test generation for software
libraries.

### Prerequisites

* CMake >= 3.8

### Building

```
$ mkdir build && cd build
$ cmake .. && make -j4
```

### Provided libraries for testing

The project currently has testing infrastructure in place for the Integer Set
Library (isl), Z3 integer API and Omega+ (comprised of a triplet of
configuration files in the `./config_files` folder). In order to run tests for a
specific library of the three, set the `config_file_path` variable in
`./src/test_emitter.cpp` to point to a particular `config_<lib>.yaml` file and
rebuild.

Tests are generated via running the `test_emitter` binary; generated test cases
will be placed in `./out/test.cpp` by default and compilation scripts are
provided in `./out/compile_<lib>.sh`.

Furthermore, `./scripts/meta_runner.py` can be used for bulk testing. Refer to
the help provided in the script for various flags to control the testing
process.

### Testing a new library

A library is described in the testing infrastructure via two *specifications*,
the *api specification* and the *metamorphic specification*. The former
describes functions and types exposed in the API under test, as well as an
optional sequence of commands to be used to generate objects of interest, while
the latter contains information about metamorphic tests to produce. The naming
convention is that these files are stored in the `./config_files` folder and
have the name `api_fuzzer_<lib>.yaml` and `meta_tests_<lib>.yaml`, where `<lib>`
represents a distinguishing identifier for the library under test.

Once the above two files are provided, a `config_<lib>.yaml` file must be be
created (see provided `config.yaml.template`), pointing to the above two
specification files and a compilation script for produced test cases. Finally,
the path to the new configuration file must be set in `./src/test_emitter.cpp`
(namely the `config_file_path` variable at the beginning of the file), and the
project rebuilt.

### Docker initMetaGenerators

Docker images are built automatically from them `master` branch at every push,
and are stored in the `alascu/metalib` repository on Docker Hub, available
[here](https://cloud.docker.com/repository/docker/alascu/metalib). Images are
tagged based on the library which they target.

The latest version of an image with the tag `<tag>` can be downloaded and executed via the following sequence of commands:
```
$ docker pull alascu/metalib:<tag>
$ docker run -ti -aSTDOUT -v=/<local_output_folder>/:/output/ alascu/metalib:<tag>
```
In the above, `<local_output_folder>` refers to a folder on the host in which to
store output from the testing process from the container (**CAUTION:** linking
host folders with container folders is essentially providing root access to the
host)

The image can be internally inspected and potentially modified by appending
`/bin/bash` to the `docker run` command, and after appropriate changes are made,
the testing process can be executed by invoking from within the container the
`meta_runner` script. For completion, the invocation encoded in the images is
(where `<lib>` represents an identifier for the library under test):
```
$ python3.7 ./scripts/meta_runner.py continuous --append-id --config-file ./config_files/config_<lib>.yaml
```
