CFLAGS=-w -s -static -O2 
COMPILER=gcc

#all: kepler_client kepler_server
all: kepler_server kepler_server_patch_1

kepler_server:
	$(COMPILER) $(CFLAGS) src/kepler_server.c -o build/kepler_server -lm

kepler_server_patch_1:
	$(COMPILER) $(CFLAGS) src/kepler_server_source_patch_1.c -o build/kepler_server_patch_1 -lm

clean:
	rm -f *.o

.PHONY: all vuln patch clean
