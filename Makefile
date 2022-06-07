VERSION=		0.5
CUDA=			$(shell dirname `dirname \`which nvcc\``)
CUDA_INCLUDE=		$(shell dirname `find $(CUDA) -name cuda.h`)
CUDA_LIBDIR=		$(shell dirname `find $(CUDA) -name libcuda.so`|head -n1)
NVRTC_INCLUDE=		$(shell dirname `find $(CUDA) -name nvrtc.h`)
NVRTC_LIBDIR=		$(shell dirname `find $(CUDA) -name libnvrtc.so`|head -n1)
ARCH=			$(shell arch)
CC=			gcc
CXX=			g++ #-Wno-deprecated-declarations
INCLUDES=		-I.
INCLUDES+=		-I$(CUDA_INCLUDE) -I$(NVRTC_INCLUDE) -I/usr/local/include

CXXFLAGS+=		-std=c++11 -O3 -g -fpic -fopenmp $(INCLUDES) -DNDEBUG
# hashpipe FLAGS
CXXFLAGS+=        -shared -lstdc++ -msse4 \
                     -L. -L/usr/local/lib \
                     -lhashpipe -lrt -lm

#added by weiyu
# LIBS+=        -Wl,-rpath=. -Llibtcc -ltcc -libverbs -lboost_system -lspead2 -ldl
LIBS+=        -Wl,-rpath=.  -libverbs -lboost_system -ldl



##changed by weiyu


HPXengine_LIB_SOURCES  = Xengine_net_thread.cc \
		      Xengine_gpu_thread.cc \
		      Xengine_output_thread.cc \
                      Xengine_databuf.cc


#hashpipe
HPXengine_LIB_OBJECTS=$(HPXengine_LIB_SOURCES:%.cc=%.o)



#Xengine_LIB_TARGET=Xengine_hashpipe.so

Xengine_LIB_TARGET=Spead2_hashpipe.so

# gpu thread
#Xengine_gpu_thread.o: libtcc/TCCorrelator.cu  Xengine_reorder.h  Xengine_reorder.cc
#	$(NVCC) -c -o $@ $< $(NVCCFLAGS)

%.d:			%.cc
			-$(CXX) $(CXXFLAGS) -MM -MT $@ -MT ${@:%.d=%.o} -MT ${@:%.d=%.s} $< -o $@


%.o:			%.cc
			$(CXX) $(CXXFLAGS) -o $@ -c $<


%.s:			%.cc
			$(CXX) $(CXXFLAGS) -o $@ -S $<



all: $(Xengine_LIB_TARGET)

#need to be checked again
$(Xengine_LIB_TARGET): $(HPXengine_LIB_OBJECTS) /usr/local/lib/libspead2.so
	$(CXX) -shared -o $@ $^ $(LIBS) $(CXXFLAGS)



clean:
	rm -f *.o
	rm -f $(Xengine_LIB_TARGET)
	rm -f *.d



prefix=/usr/local
LIBDIR=$(prefix)/lib
BINDIR=$(prefix)/bin
install-lib: $(Xengine_LIB_TARGET)
	mkdir -p "$(DESTDIR)$(LIBDIR)"
	install -p $^ "$(DESTDIR)$(LIBDIR)"
install: install-lib

.PHONY: all tags clean install install-lib
