OBJDIR = obj
BINDIR = bin
CC     = gcc 
NVCC =nvcc -arch=compute_20 -code=sm_20 #--compiler-options -fpermissive
CUDA_INSTALL_PATH	:= /opt/common/cuda/cudatoolkit-4.2.9
DRIVER_DIR = src/driver

OPT = -O3 #-g -G #--compiler-options -fpermissive #-finput-charset=charset #-fexec-charset=charset

NVCCFLAGS = $(OPT) -lz -use_fast_math -I. -I./src/shared -I$(CUDA_INSTALL_PATH)/include 
CFLAGS = $(OPT) -Wall -Wno-format -I./src/shared
LFLAGS = -fPIC -lm  -L$(CUDA_INSTALL_PATH)/lib64 -lcudart -lcuda 
DEPS   =  $(wildcard *.h)
SOURCES = $(wildcard $(DRIVER_DIR)/*.c)
CUSOURCES = $(wildcard $(DRIVER_DIR)/*.cu)
OBJS    = $(SOURCES:%.c=$(OBJDIR)/%.o)
CUOBJS  = $(CUSOURCES:%.cu=$(OBJDIR)/%.cu_o)


#CC = nvcc -lz -lm -O3 $(HEADERS) #-fomit-frame-pointer -pipe



ETAGSCMD = rm -f TAGS; find . -name '*.c' -o -name '*.h' | xargs etags
TARGET = $(BINDIR)/retrieval.gpu

.PHONY: $(TARGET) clean clean-all

all : $(OBJDIR) $(BINDIR) $(TARGET)

$(TARGET): $(OBJS) $(CUOBJS)
		@echo
		@echo Linking ...
		$(CC) $(LFLAGS) -o $@ $(OBJS) $(CUOBJS)
		@rm -f *.linkinfo
		@$(ETAGSCMD)


$(OBJS): $(OBJDIR)/%.o: %.c $(DEPS)
		$(CC) $(CFLAGS) -c $< -o $@

$(CUOBJS): $(OBJDIR)/%.cu_o: %.cu $(DEPS)
		$(NVCC) $(NVCCFLAGS) -c $< -o $@

$(OBJDIR):
		@if test ! -d $(OBJDIR); then mkdir $(OBJDIR); fi

$(BINDIR):
		@if test ! -d $(BINDIR); then mkdir $(BINDIR); fi

objects: $(OBJDIR) $(OBJS) $(CUOBJS)

clean:
		@rm -f $(OBJDIR)/*.o $(OBJDIR)/*.cu_o *~ *.linkinfo $(TARGET)

clean-all:
		@rm -rf $(OBJDIR) $(BINDIR) *~ *.linkinfo TAGS

tags:
		@$(ETAGSCMD)
