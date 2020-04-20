CC = g++

OBJ = Recorder/obj Grid/obj

.PHONY: all clean core grid recorder

all: grid recorder

grid: 
	$(MAKE) -C Grid

recorder: 
	$(MAKE) -C Recorder


clean:
	$(MAKE) -C Grid clean 
	$(MAKE) -C Recorder clean 

