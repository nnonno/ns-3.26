# ns-3.26
3.26 plus nrel-app


## Install
Download C++ library `boost` and unzip to folder `/usr/local/`
Set gcc flag to `Wall` mode before compiling NS-3
Compile NS-3 source code
Execute script of a experiment

*Example code is shown below*: 
```
sudo tar --bzip2 -xf boost_1_61_0.tar.bz2 -C /usr/local/
CXXFLAGS="-Wall" ./waf configure --enable-sudo
./waf build
./waf --run scratch/hybrid5links-updateV10
```

## TBD
