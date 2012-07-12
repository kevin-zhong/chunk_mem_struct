CCTOOL = gcc
CXXTOOL = g++
LINKTOOL = g++
ARTOOL = ar

NOW_PATH = $(shell pwd)

UNIT_TESOR = unit_tesor

FLAG_FILE = ".make_flag"

ifeq ("$(_cxxflags_)", "")

ifeq ("$(BUILD_CXXFLAGS)", "")
else

ifeq ("$(MAKECMDGOALS)", "")
make_flag = $(shell cat $(FLAG_FILE))
else
make_flag = $(MAKECMDGOALS)
endif

$(warning "************************** make_flag = $(make_flag) **************\n")
ifeq ("$(make_flag)", "build")
CXXFLAGS = $(BUILD_CXXFLAGS)
else
CXXFLAGS = $(TEST_CXXFLAGS)
endif

endif

export _cxxflags_ = $(CXXFLAGS)
else
CXXFLAGS = $(_cxxflags_)
endif

CHCK_RET = if [ $$? == 0 ]; then echo -e "\n****_success_****\n"; else echo -e "\n****_failed_****\n"

ifeq ("$(UT_CMD)", "")
	UT_CMD = ./unit_tesor > /tmp/test; $(CHCK_RET); tail -25 /tmp/test; exit -1; fi
endif

test:
	@if ! [ `cat $(FLAG_FILE)` == "$@" ]; then echo "$@" > $(FLAG_FILE); echo "transfer to <<<$@>>"; make clean; fi
	@make all
	@/usr/local/include/gtest/check_unit_testor.sh $(NOW_PATH) $(UNIT_TESOR) 1; if [ $$? == 0 ]; then make $(UNIT_TESOR);fi
	$(UT_CMD)

build:
	@if ! [ `cat $(FLAG_FILE)` == "$@" ]; then echo "$@" > $(FLAG_FILE); echo "transfer to <<<$@>>"; make clean; fi
	@make all

ALL_OBJS = $(OBJS) $(UNIT_TESOR).o
ALL_DS = $(OBJS:.o=.d)
sinclude $(ALL_DS)


$(UNIT_TESOR): $(OBJS) $(UNIT_TESOR).o
	@echo Achiving [$(UNIT_TESOR)]
	$(CXXTOOL) -o $@ $^ $(CXXFLAGS) -lgtest $(LDFLAGS)

rm_c :
	@rm $(ALL_DS) -rf
	@rm $(UNIT_TESOR)* -rf

$(ALL_DS): %.d: %.cpp
	@echo "create $@"
	@set -e; rm -f $@*; \
	$(CXXTOOL) -MM  $< > $@.$$$$ $(INC); \
	sed 's:.*\.o:$< $@ :' $@.$$$$ | sed 's:\.cpp:\.o:' > $@; \
	rm -f $@.$$$$

