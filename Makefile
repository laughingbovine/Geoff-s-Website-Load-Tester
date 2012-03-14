# legend:
# % 	- wildcard ("stem")
# $@ 	- the target (thing before the colon)
# $< 	- the first prerequisite (the first thing after the colon)
# $^ 	- all prerequisites (the things after the colon)

all: loadtest tests

################################################################################

loadtest: obj/utils.o obj/tcp.o obj/test.o obj/loadtest.o
	g++ -lpthread -o $@ $^

################################################################################

tests: utils.test tcp.test test.test

utils.test: obj/utils.o obj/test/utils.o
	g++ -o $@ $^

tcp.test: obj/utils.o obj/tcp.o obj/test/tcp.o
	g++ -o $@ $^

test.test: obj/utils.o obj/tcp.o obj/test.o obj/test/test.o
	g++ -lpthread -o $@ $^

################################################################################

clean: cleano cleanx

# clean all object files
cleano:
	find obj/ -type f -name "*.o" -exec rm {} \;

# clean all executibles
cleanx:
	find . -maxdepth 1 -type f -name "loadtest" -or -name "*.test" -exec rm {} \;

################################################################################

obj/%.o: src/%.cpp src/%.h
	g++ -c $< -o $@

# and for .cpp's without a corresponding .h:
obj/%.o: src/%.cpp
	g++ -c $< -o $@
