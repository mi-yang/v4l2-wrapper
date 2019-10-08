SDK_DIR=/home/mi/sandbox/rk3288_sdk
TOOL_DIR=$(SDK_DIR)/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf
CC=$(TOOL_DIR)/bin/arm-linux-gnueabihf-g++
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
LIBS := -lpthread 

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
	$(CC) -o $@ $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
$(OBJS):$(SRCS)
	@echo 'Building target: $@'
	$(CC) $(CFLAGS) -c $(filter %$(patsubst %o,%cpp,$(notdir $@)),$(SRCS)) -o $@

clean:
	@echo "make" $(TARGET) "clean"
	rm -f $(OBJDIR)/*.a $(OBJDIR)/*.o $(TARGET)
