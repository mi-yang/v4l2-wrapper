CC=g++
ROOT=.
#PROJDIR=$(ROOT)/..
OBJDIR=$(ROOT)/obj
TARGET=$(OBJDIR)/v4l2Sample
 
exclude_dirs:=obj
exclude_dirs+=Debug

SUBDIR:=$(filter-out $(exclude_dirs),$(basename $(patsubst ./%,%,$(shell find . -maxdepth 1 -type d))))
#incs:=$(foreach dir,$(SUBDIR),$(shell ls $(dir)/inc/*.h))
SRCS:=$(foreach dir,$(SUBDIR),$(shell ls $(dir)/*.cpp))
file:=$(notdir $(SRCS))
OBJS:=$(patsubst %.cpp,$(OBJDIR)/%.o,$(file))
#temp:=$(filter %BusUart.cpp,$(SRCS))
 
#ifeq ($(DEBUG),1)
#        CFLAGS=-Wall -DDBUG
#else
#        CFLAGS=-Wall -DRELEASE
#endif
 
#INCS=-I$(PROJDIR)/common/inc
LDFLAGS=
LIBS := -lopencv_core -lpthread -lopencv_highgui

CFLAGS+=$(INCS)
 
.PHONY: all clean
all:OUT_DIR $(TARGET)

OUT_DIR:
	@echo "create output dir obj"
	$(shell mkdir -p obj)
	@echo "done"

$(TARGET):$(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o $@ $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
$(OBJS):$(SRCS)
	@echo 'Building target: $@'
	$(CC) $(CFLAGS) -c $(filter %$(patsubst %o,%cpp,$(notdir $@)),$(SRCS)) -o $@

clean:
	@echo "make" $(TARGET) "clean"
	rm -f $(OBJDIR)/*.a $(OBJDIR)/*.o
