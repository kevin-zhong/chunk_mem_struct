TARGET ?= libcomm.a
SRCDIR ?= .
TASK ?= $(TARGET)

################################################################################
#objects
SRC_FILES = $(shell find . -name '*.cpp'  | grep -v $(UNIT_TESOR) | grep -v lua.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRC_FILES))

################################################################################
INC = -I./ \
	-I/usr/local/include/ \
	-I/usr/local/include/comm \
	-I/usr/local/include/thrift \
	-I/usr/local/include/xl_lib/\
	

#DEBUGFLAGS = -O3
DEBUGFLAGS = -g -D_DEBUG
CCFLAGS = -Wall
ARFLAGS = r

CCFLAGS += $(DEBUGFLAGS) $(INC)

TEST_CXXFLAGS = $(CCFLAGS) -D__cplusplus -D_LOG_USE_STDOUT -D__LINUX__ -D_NO_GLOBAL_H  -D_THREADS
BUILD_CXXFLAGS = $(CCFLAGS) -D__cplusplus -D_LOG_USE_LOG4CPLUS -D__LINUX__ -D_THREADS

LDFLAGS = -lssl -lpthread -luuid /usr/local/lib/libsnappy.a \
		  /usr/local/lib/libthriftz.a /usr/local/lib/libthrift.a
			##/usr/local/lib/liblog4cplus.a

################################################################################

all: 
	@echo "all objs : " $(OBJS)
	make -C protocal
	make $(TASK)

include $(HOME)/comm/test_make.inc

UT_CMD += ;rm user_cid_*

mem_test:
	valgrind --tool=memcheck ./unit_tesor > /tmp/test

$(TASK): $(OBJS)
	@echo Achiving [$(TARGET)]
	$(ARTOOL) $(ARFLAGS) $(TARGET) $(OBJS)

clean:
	echo Removing object files
	@make rm_c
	@rm $(OBJS) $(TARGET) -rf
##@make -C protocal clean

