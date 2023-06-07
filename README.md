# Trifecta

This is a software to benchmark Trifecta, a faster high-throughput three-party computation protocol. This is as part of the submission for PoPETS 23 where the paper will be presented.

The implementation here is a fork of [MP-SPDZ](https://github.com/data61/MP-SPDZ/tree/master), a general toolkit to prototype and benchmark various multi-party computation (MPC) protocol. The vanilla MP-SPDZ doesn't support computations on multi-fan-in AND gates, required by protocols such as Trifecta. Therefore, we make multiple improvements in the code to enable this feature:

1. We modify the built-in MP-SPDZ compiler to accept representations of Boolean circuits with multi-fan-in AND gates. The compiler thus generates a bytecode that is augmented to  include multi-fan-in instructions. For our use-case, we extend the [Bristol Fashion](https://homes.esat.kuleuven.be/~nsmart/MPC/) circuit format.
2. Subsequently, the virtual machine is enhanced to parse and process the new multi-fan-in instructions in a backward-compatible manner.
3. We implement Trifecta on top of this upgraded toolchain for experimental evalution of our protocol and verify our build.

We note that the changes introduced here are not specific to Trifecta and the underlying framework can be used to benchmark any protocol that can compute multi-fan-in AND gates.

# <a name="compilation"></a> Compilation

### Docker

Build a docker image with all the dependencies and required player data with the command below

```
docker build --tag mpspdz:trifecta .
```

Skip to the [next section](#computation) for how to run a program.

Alternatively, you can follow the instructions in the [MP-SPDZ](https://github.com/data61/MP-SPDZ/tree/master) repository to install all the requirements. Also see the [documentation](https://mp-spdz.readthedocs.io/en/latest/index.html) for FAQs and common issues.

On an Ubuntu machine, the following one-liner should suffice:

```
sudo apt-get install automake build-essential clang cmake git libboost-dev libboost-thread-dev libgmp-dev libntl-dev libsodium-dev libssl-dev libtool python3 yasm m4
```

Since this is a fork of an older release of MP-SPDZ, there is a nuanced additional step to configure the software. We rely on the [MPIR Library](https://github.com/wbhart/mpir) for integer operations. The MPIR folder contains the latest release of the library provided here for convinience. To complete the installation

```
cd mpir

./configure --enable-cxx 
make

make check && make install
ldconfig
```

Finally, run the following command to compile the binary for the Trifecta machine in the main directory

```
make multi-replicated-bin-party.x
```

You can speed up this last step by adding ``` -j8 ``` flag to the previous command.

Trifecta requires private communication between the parties to prevent eavesdroppers from reconstructing the secret. MP-SPDZ uses OpenSSL to establish secure channels. To generate the necessary certificates and keys run

```
Scripts/setup-ssl.sh [<number of parties>]
c_rehash Player-Data
```
To run the protocol on separate instances over WAN, specify the IP address and port number of each machine inside ```Player-Data/ip-file``` with the following format

```
<host0>[:<port0>]
<host1>[:<port1>]
...
```

# <a name="computation"></a> Running Computations

To run a computation, you first need to compile the source code. For example ```rc_adder.mpc ``` program is as follows

```
from circuit import Circuit
sb64 = sbits.get_type(64)
adder = Circuit('rc_adder_64')
a = sbitvec(sb64.get_input_from(0))
b = sbitvec(sb64.get_input_from(0))
print_ln('%s', adder(a, b).elements()[0].reveal())
```

This will take two 64-bit numbers as input from party 0, adds them using a ripple-carry adder, printing the result of the computation. To compile the program run

```
./compile.py -B 64 rc_adder
```

where ``` -B ``` indicates the bit-length of the input. Then to start the computation on the same machine, run

``` 
./multi-replicated-bin-party -I -p 0 rc_adder
```

``` 
./multi-replicated-bin-party -I -p 1 rc_adder
```

``` 
./multi-replicated-bin-party -I -p 2 rc_adder
```

in separate terminals. To run on separate machines, use the same command with ``` --ip-file-name Player-Data/ip-file ``` where ip-file is as described in [Compilation](#compilation). The ```-I``` flag prompts the user for inputs in an interactive mode. Omit this flag to read inputs from ``` Player-Data/Input-P<party number>-0 ``` in text format.

We have provided our depth-optimized circuits using multi-fan-in AND gates for multiple functionalities in ``` Programs/Circuits ```. Follow the same instructions to compile and run a new computation by simply substituting the circuit in the source code with the desired one and adjusting the input bit-length.

You can find more examples and template programs in ``` Programs/Source ```. 
