
include CONFIG

MATH = $(patsubst %.cpp,%.o,$(wildcard Math/*.cpp))

TOOLS = $(patsubst %.cpp,%.o,$(wildcard Tools/*.cpp))

NETWORK = $(patsubst %.cpp,%.o,$(wildcard Networking/*.cpp))

PROCESSOR = $(patsubst %.cpp,%.o,$(wildcard Processor/*.cpp))

FHEOFFLINE = $(patsubst %.cpp,%.o,$(wildcard FHEOffline/*.cpp FHE/*.cpp)) Protocols/CowGearOptions.o

GC = $(patsubst %.cpp,%.o,$(wildcard GC/*.cpp)) $(PROCESSOR)
GC_SEMI = GC/SemiSecret.o GC/SemiPrep.o GC/square64.o

OT = $(patsubst %.cpp,%.o,$(wildcard OT/*.cpp))
OT_EXE = ot.x ot-offline.x

COMMON = $(MATH) $(TOOLS) $(NETWORK) GC/square64.o Processor/OnlineOptions.o
COMPLETE = $(COMMON) $(PROCESSOR) $(FHEOFFLINE) $(TINYOTOFFLINE) $(GC) $(OT)
YAO = $(patsubst %.cpp,%.o,$(wildcard Yao/*.cpp)) $(OT) BMR/Key.o
BMR = $(patsubst %.cpp,%.o,$(wildcard BMR/*.cpp BMR/network/*.cpp)) $(COMMON) $(PROCESSOR) $(OT)
VM = $(PROCESSOR) $(COMMON) GC/square64.o OT/OTTripleSetup.o OT/BaseOT.o $(LIBSIMPLEOT)


LIB = libSPDZ.a
LIBRELEASE = librelease.a
LIBSIMPLEOT = SimpleOT/libsimpleot.a

# used for dependency generation
OBJS = $(BMR) $(FHEOFFLINE) $(TINYOTOFFLINE) $(YAO) $(COMPLETE) $(patsubst %.cpp,%.o,$(wildcard Machines/*.cpp))
DEPS := $(wildcard */*.d */*/*.d)

# never delete
.SECONDARY: $(OBJS)


all: arithmetic binary gen_input online offline externalIO bmr doc

.PHONY: doc
doc:
	cd doc; $(MAKE) html

arithmetic: rep-ring rep-field shamir semi2k-party.x semi-party.x mascot
binary: rep-bin yao semi-bin-party.x tinier-party.x tiny-party.x ccd-party.x malicious-ccd-party.x real-bmr

ifeq ($(USE_NTL),1)
all: overdrive she-offline
gear: cowgear-party.x chaigear-party.x
arithmetic: hemi-party.x soho-party.x gear
endif

-include $(DEPS)
include $(wildcard *.d static/*.d)

%.o: %.cpp
	$(CXX) -o $@ $< $(CFLAGS) -MMD -MP -c

online: Fake-Offline.x Server.x Player-Online.x Check-Offline.x

offline: $(OT_EXE) Check-Offline.x

gen_input: gen_input_f2n.x gen_input_fp.x

externalIO: bankers-bonus-client.x

bmr: bmr-program-party.x bmr-program-tparty.x

real-bmr: $(patsubst Machines/%.cpp,%.x,$(wildcard Machines/*-bmr-party.cpp))

yao: yao-party.x

she-offline: Check-Offline.x spdz2-offline.x

overdrive: simple-offline.x pairwise-offline.x cnc-offline.x

rep-field: malicious-rep-field-party.x replicated-field-party.x ps-rep-field-party.x

rep-ring: replicated-ring-party.x brain-party.x malicious-rep-ring-party.x ps-rep-ring-party.x Fake-Offline.x

rep-bin: replicated-bin-party.x malicious-rep-bin-party.x Fake-Offline.x

replicated: rep-field rep-ring rep-bin

spdz2k: spdz2k-party.x ot-offline.x Check-Offline-Z2k.x galois-degree.x Fake-Offline.x
mascot: mascot-party.x spdz2k mama-party.x

tldr:
	-echo ARCH = -march=native >> CONFIG.mine
	$(MAKE) mascot-party.x

ifeq ($(OS), Darwin)
tldr: mac-setup
else
tldr: mpir
endif

shamir: shamir-party.x malicious-shamir-party.x galois-degree.x

ecdsa: $(patsubst ECDSA/%.cpp,%.x,$(wildcard ECDSA/*-ecdsa-party.cpp))
ecdsa-static: static-dir $(patsubst ECDSA/%.cpp,static/%.x,$(wildcard ECDSA/*-ecdsa-party.cpp))

$(LIBRELEASE): $(patsubst %.cpp,%.o,$(wildcard Protocols/*.cpp)) $(PROCESSOR) $(COMMON) $(BMR) $(FHEOFFLINE) $(GC)
	$(AR) -csr $@ $^

static/%.x: Machines/%.o $(LIBRELEASE) $(LIBSIMPLEOT)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBRELEASE) $(LIBSIMPLEOT) -Wl,-Map=$<.map -Wl,-Bstatic -static-libgcc -static-libstdc++ $(BOOST) $(LDLIBS) -Wl,-Bdynamic -ldl

static/%.x: ECDSA/%.o ECDSA/P256Element.o $(VM) $(OT) $(LIBSIMPLEOT)
	$(CXX) $(CFLAGS) -o $@ $^ -Wl,-Map=$<.map -Wl,-Bstatic -static-libgcc -static-libstdc++ $(BOOST) $(LDLIBS) $(ECLIB) -Wl,-Bdynamic -ldl

static-dir:
	@ mkdir static 2> /dev/null; true

static-release: static-dir $(patsubst Machines/%.cpp, static/%.x, $(wildcard Machines/*-party.cpp))

Fake-ECDSA.x: ECDSA/Fake-ECDSA.cpp ECDSA/P256Element.o $(COMMON)
	$(CXX) -o $@ $^ $(CFLAGS) $(LDLIBS) $(ECLIB)

Check-Offline.x: $(PROCESSOR)

ot.x: $(OT) $(COMMON) Machines/OText_main.o Machines/OTMachine.o $(LIBSIMPLEOT)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDLIBS)

ot-offline.x: $(OT) $(LIBSIMPLEOT) Machines/TripleMachine.o

gc-emulate.x: $(PROCESSOR) GC/FakeSecret.o GC/square64.o

bmr-%.x: $(BMR) Machines/bmr-%.cpp $(LIBSIMPLEOT)
	$(CXX) -o $@ $(CFLAGS) $^ $(BOOST) $(LDLIBS)

%-bmr-party.x: Machines/%-bmr-party.o $(BMR) $(LIBSIMPLEOT)
	$(CXX) -o $@ $(CFLAGS) $^ $(BOOST) $(LDLIBS)

bmr-clean:
	-rm BMR/*.o BMR/*/*.o GC/*.o

bankers-bonus-client.x: ExternalIO/bankers-bonus-client.cpp $(COMMON)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDLIBS)

simple-offline.x: $(FHEOFFLINE)
pairwise-offline.x: $(FHEOFFLINE)
cnc-offline.x: $(FHEOFFLINE)
spdz2-offline.x: $(FHEOFFLINE)

yao-party.x: $(YAO)
static/yao-party.x: $(YAO)

yao-clean:
	-rm Yao/*.o

galois-degree.x: Utils/galois-degree.o
	$(CXX) $(CFLAGS) -o $@ $^ $(LDLIBS)

default-prime-length.x: Utils/default-prime-length.o
	$(CXX) $(CFLAGS) -o $@ $^ $(LDLIBS)

secure.x: Utils/secure.o
	$(CXX) -o $@ $(CFLAGS) $^

%.x: Utils/%.o $(COMMON)
	$(CXX) -o $@ $(CFLAGS) $^ $(LDLIBS)

%.x: Machines/%.o $(VM) OT/OTTripleSetup.o OT/BaseOT.o $(LIBSIMPLEOT)
	$(CXX) -o $@ $(CFLAGS) $^ $(LDLIBS)

%-ecdsa-party.x: ECDSA/%-ecdsa-party.o ECDSA/P256Element.o $(VM)
	$(CXX) -o $@ $(CFLAGS) $^ $(LDLIBS) $(ECLIB)

replicated-bin-party.x: GC/square64.o
multi-replicated-bin-party.x: GC/square64.o
replicated-ring-party.x: GC/square64.o
replicated-field-party.x: GC/square64.o
brain-party.x: GC/square64.o
malicious-rep-bin-party.x: GC/square64.o
semi-bin-party.x: $(VM) $(OT) GC/SemiSecret.o GC/SemiPrep.o GC/square64.o
tiny-party.x: $(OT)
tinier-party.x: $(OT)
spdz2k-party.x: $(OT) $(patsubst %.cpp,%.o,$(wildcard Machines/SPDZ2*.cpp))
static/spdz2k-party.x: $(patsubst %.cpp,%.o,$(wildcard Machines/SPDZ2*.cpp))
semi-party.x: $(OT) GC/SemiSecret.o GC/SemiPrep.o GC/square64.o
semi2k-party.x: $(OT) GC/SemiSecret.o GC/SemiPrep.o GC/square64.o
hemi-party.x: $(FHEOFFLINE) $(GC_SEMI) $(OT)
soho-party.x: $(FHEOFFLINE) $(GC_SEMI) $(OT)
cowgear-party.x: $(FHEOFFLINE) Protocols/CowGearOptions.o $(OT)
chaigear-party.x: $(FHEOFFLINE) Protocols/CowGearOptions.o $(OT)
mascot-party.x: Machines/SPDZ.o $(OT)
static/mascot-party.x: Machines/SPDZ.o
Player-Online.x: Machines/SPDZ.o $(OT)
mama-party.x: $(OT)
ps-rep-ring-party.x: Protocols/MalRepRingOptions.o
malicious-rep-ring-party.x: Protocols/MalRepRingOptions.o
semi-ecdsa-party.x: $(OT) $(LIBSIMPLEOT) GC/SemiPrep.o GC/SemiSecret.o
mascot-ecdsa-party.x: $(OT) $(LIBSIMPLEOT)
fake-spdz-ecdsa-party.x: $(OT) $(LIBSIMPLEOT)

$(LIBSIMPLEOT): SimpleOT/Makefile
	$(MAKE) -C SimpleOT

OT/BaseOT.o: SimpleOT/Makefile

SimpleOT/Makefile:
	git submodule update --init SimpleOT

.PHONY: Programs/Circuits
Programs/Circuits:
	git submodule update --init Programs/Circuits

.PHONY: mpir-setup mpir-global mpir
mpir-setup:
	#git submodule update --init mpir
	cd mpir; \
	autoreconf -i; \
	autoreconf -i
	- $(MAKE) -C mpir clean

mpir-global: mpir-setup
	cd mpir; \
	./configure --enable-cxx;
	$(MAKE) -C mpir
	sudo $(MAKE) -C mpir install

mpir: mpir-setup
	cd mpir; \
	./configure --enable-cxx --prefix=$(CURDIR)/local
	$(MAKE) -C mpir install
	-echo MY_CFLAGS += -I./local/include >> CONFIG.mine
	-echo MY_LDLIBS += -Wl,-rpath -Wl,./local/lib -L./local/lib >> CONFIG.mine

mac-setup:
	brew install openssl boost libsodium mpir yasm ntl
	-echo MY_CFLAGS += -I/usr/local/opt/openssl/include >> CONFIG.mine
	-echo MY_LDLIBS += -L/usr/local/opt/openssl/lib >> CONFIG.mine
	-echo USE_NTL = 1 >> CONFIG.mine

clean:
	-rm */*.o *.o */*.d *.d *.x core.* *.a gmon.out */*/*.o static/*.x
