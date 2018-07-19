UNAME := $(shell uname)
UNAME_M := $(shell uname -m)
GIT_BRANCH := $(shell git branch | head -n 1 | awk '{print $2}')

$(info $$UNAME_M = [${UNAME_M}])
$(info $$GIT_BRANCH = [${GIT_BRANCH}])

CFLAGS = -std=gnu99 -O2
ifeq ($(GIT_BRANCH), * beta)
CFLAGS += -ggdb
endif

CFLAGS += -march=native -mfpmath=sse -Wall -Wno-unknown-pragmas
CFLAGS += -I headers -I /usr/local/include -I /usr/include -fPIC

ifeq ($(UNAME), Darwin)
CFLAGS += -fms-extensions -Wno-microsoft
endif

# Some heavy debuggning flags
#CFLAGS += -DDEBUG_IIR
#CFLAGS += -DDEBUG_IQ
#CFLAGS += -DDEBUG_FILE_MANAGER

LDFLAGS = -L ./ -L /usr/local/lib

OBJS = RadarKit.o RKRadar.o RKCommandCenter.o RKTest.o
OBJS += RKFoundation.o RKMisc.o RKDSP.o RKSIMD.o RKClock.o RKWindow.o
OBJS += RKPreference.o
OBJS += RKFileManager.o RKHostMonitor.o
OBJS += RKConfig.o RKHealth.o
OBJS += RKPulseCompression.o RKPulseRingFilter.o RKMoment.o
OBJS += RKRadarRelay.o
OBJS += RKNetwork.o RKServer.o RKClient.o
OBJS += RKPulsePair.o RKMultiLag.o
OBJS += RKPosition.o
OBJS += RKHealthRelayTweeta.o RKPedestalPedzy.o
OBJS += RKRawDataRecorder.o RKSweep.o RKSweepFile.o RKProduct.o RKProductFile.o RKHealthLogger.o
OBJS += RKWaveform.o

RKLIB = libradarkit.a

PROGS = rktest simple-emulator

ifeq ($(UNAME), Darwin)
# Mac OS X
CC = clang
CFLAGS += -D_DARWIN_C_SOURCE -Wno-deprecated-declarations -mmacosx-version-min=10.9
else
# Old Debian
ifeq ($(UNAME_M), i686)
CFLAGS += -D_GNU_SOURCE -D_EXPLICIT_INTRINSIC -msse -msse2 -msse3 -msse4 -msse4.1
LDFLAGS += -L /usr/lib64
else
CFLAGS += -D_GNU_SOURCE
LDFLAGS += -L /usr/lib64
endif
endif

#LDFLAGS += -Wl,-Bstatic -lradarkit -lfftw3f -lnetcdf -Wl,-Bdynamic -lpthread -lz -lm
LDFLAGS += -lradarkit -lfftw3f -lnetcdf -lpthread -lz -lm

ifeq ($(UNAME), Darwin)
else
LDFLAGS += -lrt
endif

#all: $(RKLIB) install rktest
all: $(RKLIB) $(PROGS)

$(OBJS): %.o: source/%.c
	$(CC) $(CFLAGS) -I headers/ -c $< -o $@

$(RKLIB): $(OBJS)
	ar rvcs $@ $(OBJS)

rktest: RadarKitTest/main.c libradarkit.a
	$(CC) -o $@ $(CFLAGS) $< $(LDFLAGS)
# rktest: RadarKitTest/main.c libradarkit.a
# 	$(CC) -o $@ $(CFLAGS) $< $(OBJS) $(LDFLAGS)

simple-emulator: SimpleEmulator/main.c libradarkit.a
	$(CC) -o $@ $(CFLAGS) $< $(LDFLAGS)
# simple-emulator: SimpleEmulator/main.c libradarkit.a
# 	$(CC) -o $@ $(CFLAGS) $< $(OBJS) $(LDFLAGS)

clean:
	rm -f $(PROGS)
	rm -f $(RKLIB)
	rm $(OBJS)

install:
	sudo cp -rp headers/RadarKit headers/RadarKit.h /usr/local/include/
	sudo cp -p libradarkit.a /usr/local/lib/

uninstall:
	rm -rf /usr/local/include/RadarKit.h /usr/local/include/RadarKit
	rm -rf /usr/local/lib/libradarkit.a
