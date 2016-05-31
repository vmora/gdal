
include ../../GDALmake.opt

CPPFLAGS := $(PYTHON_INCLUDE) $(CPPFLAGS)

OBJ = python_dataset.o

default: $(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o

install-obj: $(O_OBJ:.o=.$(OBJ_EXT))
