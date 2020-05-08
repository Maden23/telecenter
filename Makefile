.PHONY: all clean core grid recorder singlestream

all: grid recorder singlestream

grid: 
	$(MAKE) -C Grid

recorder: 
	$(MAKE) -C Recorder

singlestream:
	$(MAKE) -C SingleStream

clean:
	$(MAKE) -C Grid clean 
	$(MAKE) -C Recorder clean 
	$(MAKE) -C SingleStream clean

