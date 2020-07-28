prl-disp-service
================

prl-disp-service is a OpenVZ management service. It is a component of
[OpenVZ](https://openvz.org/).

OpenVZ is a powerful, easy to use, cost effective server virtualization solution
that empowers PC users with the ability to create completely networked, fully
portable, entirely independent virtual machines and Containers on a single
physical machine.

### How to contribute

* [How to submit a patch](https://openvz.org/How_to_submit_patches)
* [How to contribute to OpenVZ](https://openvz.org/Contribute)

### How to build
To build dispatcher
```
./Gen.py
(cd Libraries/Transponster && qmake-qt4 && make debug)
(cd Dispatcher && qmake-qt4 && make  -j<N> debug)
```
where is \<N\> - a number of CPU cores on the build node + 2.

To build tests:
```
cd Tests
qmake-qt4
make
```

To build in Docker container:
```
docker build -t prl-disp-service .
docker run -v "$(pwd):/root/src" -it prl-disp-service
```
