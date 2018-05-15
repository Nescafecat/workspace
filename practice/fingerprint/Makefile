#
# Copyright 2017-2019 AORE Technology Co.,Ltd. 
#	All Right Reserved.
#
# 	Yang Rongkui	
#	2018-04-24
#
# This file is EM6000(S)'s makefile
#

include ../../../../Makefile.inc

LibName := libfingerprint.a


SRCDIRS :=
LINC := -I./. -I../../inc -I../../../../inc/ -I../../../../inc/api -I../../../../inc/hal -I../../hal \
	-I../../../../inc/808 -I../../../inc/api/ -I$(INC_DIR) -I$(INC_API_DIR) -I$(INC_HAL_DIR) -I$(INC_808_DIR) -I../prot
LIBS := -L$(LIB_DIR)  -lpthread 
OUT_LIBS:=../../lib

OBJ :=$(LibName)
SRC = $(wildcard *.c) $(wildcard *.cc) $(wildcard *.cpp)

all: $(OBJ)

%.o: %.cpp
	$(CXX) $(FLAGS) -fPIC -c $< $(LINC) $(MACRO)

libfingerprint.a: $(SRC:.cpp=.o)
	$(AR) rcvs $@ $^
	cp $@ $(OUT_LIBS)/$@

depend: $(SRC)
	@$(CC) $(FLAGS) $(LINC) -w -MM $^ > $@

-include depend

.PHONY:clean install uninstall rebuild start check test tar

clean:
	-rm $(OBJS) depend
	-rm $(OBJ) *.o
	-rm $(OUT_LIBS)/$(LibName)


install:
	echo "No Support!";

uninstall:
	echo "No Support!";

rebuild: clean all

check:
	-make -t

test: check


