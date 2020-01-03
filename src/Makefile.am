bin_PROGRAMS = qperf

if RDMA
AM_CFLAGS = -Wall -O -DRDMA
if HAS_XRC
AM_CFLAGS += -DHAS_XRC=1
AM_CFLAGS += -DHAVE_FF_KQUEUE -DINET -DINET6 
AM_CFLAGS += -L${FF_PATH}/lib -L${FF_PATH}/tools/compat -lfstack -ldl
AM_CFLAGS += -L${FF_DPDK}/lib -ldpdk -ldl
endif
qperf_SOURCES = qperf.c  socket.c qperf_ff.c rds.c rdma.c support.c help.c qperf.h
qperf_LDADD = -libverbs -ldl -lfstack -ldpdk -lffcompat 
else
AM_CFLAGS = -Wall -O
AM_CFLAGS += -Wno-unsed-but-set-variable -Wno-unused-variable -DHAVE_FF_KQUEUE -DINET -DINET6 -DFSTACK -std=c99 
AM_CFLAGS += -I${FF_DPDK}/include 
AM_CFLAGS += -I${FF_PATH}/lib
AM_CFLAGS += -L${FF_PATH}/lib -ldl -Wl,--whole-archive,-lfstack,--no-whole-archive
AM_CFLAGS += -L${FF_DPDK}/lib -ldl -Wl,--whole-archive,-ldpdk,--no-whole-archive,--no-whole-archive -lrt -lm -ldl -lcrypto -lpthread -lnuma

qperf_SOURCES = qperf.c qperf_ff.c socket.c rds.c support.c help.c qperf.h
endif

man_MANS = qperf.1

qperf.1: help.txt
	./mkman >qperf.1

help.c: help.txt
if RDMA
	./mkhelp RDMA
else
	./mkhelp
endif

clean-local:
	-rm -f qperf.1
