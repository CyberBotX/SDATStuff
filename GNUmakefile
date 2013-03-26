UNAME:=	$(shell uname -s)

COMMON_SRCS=	SDAT.cpp NDSStdHeader.cpp SYMBSection.cpp INFOSection.cpp INFOEntry.cpp FATSection.cpp SSEQ.cpp SWAV.cpp SWAR.cpp SBNK.cpp
COMMON_SRCS:=	$(sort $(addprefix common/,$(COMMON_SRCS)))
TIMER_SRCS:=	$(wildcard common/Timer*.cpp)

SDATtoNCSF_SRCS:=	SDATtoNCSF/SDATtoNCSF.cpp common/TagList.cpp common/NCSF.cpp $(TIMER_SRCS) $(COMMON_SRCS)
SDATStrip_SRCS:=	SDATStrip/SDATStrip.cpp $(COMMON_SRCS)
NDStoNCSF_SRCS:=	NDStoNCSF/NDStoNCSF.cpp common/TagList.cpp common/NCSF.cpp $(TIMER_SRCS) $(COMMON_SRCS)

PROGS=	SDATtoNCSF/SDATtoNCSF SDATStrip/SDATStrip NDStoNCSF/NDStoNCSF
PROGS:=	$(sort $(PROGS))

CXX=	g++46
PROG_SUFFIX=
OBJ_SUFFIX=	.o
DEP_SUFFIX=	.d

CPPFLAGS+=	-Icommon
CXXFLAGS+=	-std=c++0x -pipe -pedantic -Wall -Wctor-dtor-privacy -Wold-style-cast -Wextra -Wno-div-by-zero -Wfloat-equal -Wshadow -Wno-long-long -Icommon
LDFLAGS+=	-lz -pthread

ifneq (,$(findstring MINGW,$(UNAME)))
PROGS:=	$(addsuffix .exe,$(PROGS))
CXX=	g++
PROG_SUFFIX=	.exe
OBJ_SUFFIX=	.wo
DEP_SUFFIX=	.wd
CXXFLAGS+=	-static-libgcc -static-libstdc++
endif

PROG_SRCS_template=	$(1)_SRCS:=	$$(sort $$($(1)_SRCS))
PROG_OBJS_template=	$(1)_OBJS:=	$$($(1)_SRCS:%.cpp=%$$(OBJ_SUFFIX))

$(foreach prog,$(PROGS),$(eval $(call PROG_SRCS_template,$(basename $(notdir $(prog))))))
$(foreach prog,$(PROGS),$(eval $(call PROG_OBJS_template,$(basename $(notdir $(prog))))))

SRCS:=	$(foreach prog,$(PROGS),$($(basename $(notdir $(prog)))_SRCS))
SRCS:=	$(sort $(SRCS))
OBJS:=	$(SRCS:%.cpp=%$(OBJ_SUFFIX))
DEPS:=	$(SRCS:%.cpp=%$(DEP_SUFFIX))

.PHONY: clean all debug

.SUFFIXES:
.SUFFIXES: .cpp $(OBJ_SUFFIX) $(DEP_SUFFIX) $(PROG_SUFFIX)

all: $(PROGS)
debug: CXXFLAGS+=	-g
debug: all

define PROG_template
$(1): $$($$(basename $$(notdir $(1)))_OBJS)
	@echo "Linking $$@..."
	@$$(CXX) $$(CXXFLAGS) -o $$@ $$^ $$(LDFLAGS)
endef

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))

%$(OBJ_SUFFIX) : %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

%$(DEP_SUFFIX) : %.cpp
	@echo "Calculating depends for $<..."
	@$(CXX) $(CPPFLAGS) -MM -MF $@.tmp $<
	@sed 's,$(notdir $*)\.o[ :]*,$(subst /,\/,$*)$(OBJ_SUFFIX) $(subst /,\/,$@): ,g' < $@.tmp > $@
	@rm -f $@.tmp

clean:
	-rm $(OBJS) $(PROGS)

-include $(DEPS)
