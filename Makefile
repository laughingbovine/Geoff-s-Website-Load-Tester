# legend:
# % 	- wildcard ("stem")
# $@ 	- the target (thing before the colon)
# $< 	- the first prerequisite (the first thing after the colon)
# $^ 	- all prerequisites (the things after the colon)

#compiler = g++ -g -O0
compiler = g++

all: loadtest tests

################################################################################

loadtest: obj/utils.o obj/tcp.o obj/test.o obj/tester.o obj/loadtest.o
	$(compiler) -lpthread -o $@ $^

################################################################################

tests: utils.test tcp.test test.test tester.test

utils.test: obj/utils.o obj/test/utils.o
	$(compiler) -o $@ $^

tcp.test: obj/utils.o obj/tcp.o obj/test/tcp.o
	$(compiler) -o $@ $^

test.test: obj/utils.o obj/tcp.o obj/test.o obj/test/test.o
	$(compiler) -lpthread -o $@ $^

tester.test: obj/utils.o obj/tcp.o obj/test.o obj/tester.o obj/test/tester.o
	$(compiler) -lpthread -o $@ $^

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
	@mkdir -p $(@D)
	$(compiler) -c $< -o $@

# and for .cpp's without a corresponding .h:
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(compiler) -c $< -o $@
