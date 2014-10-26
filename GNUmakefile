UNAME:=	$(shell uname -s)

SRCDIR:=	$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

COMMON_SRCS=	SDAT.cpp NDSStdHeader.cpp SYMBSection.cpp INFOSection.cpp INFOEntry.cpp FATSection.cpp SSEQ.cpp SWAV.cpp SWAR.cpp SBNK.cpp
COMMON_SRCS:=	$(sort $(addprefix $(SRCDIR)common/,$(COMMON_SRCS)))
TIMER_SRCS:=	$(wildcard $(SRCDIR)common/Timer*.cpp)

SDATtoNCSF_SRCS:=	$(SRCDIR)SDATtoNCSF/SDATtoNCSF.cpp $(SRCDIR)common/TagList.cpp $(SRCDIR)common/NCSF.cpp $(TIMER_SRCS) $(COMMON_SRCS)
SDATStrip_SRCS:=	$(SRCDIR)SDATStrip/SDATStrip.cpp $(COMMON_SRCS)
NDStoNCSF_SRCS:=	$(SRCDIR)NDStoNCSF/NDStoNCSF.cpp $(SRCDIR)common/TagList.cpp $(SRCDIR)common/NCSF.cpp $(TIMER_SRCS) $(COMMON_SRCS)
2SFTagsToNCSF_SRCS:=	$(SRCDIR)2SFTagsToNCSF/2SFTagsToNCSF.cpp $(SRCDIR)common/TagList.cpp $(SRCDIR)common/NCSF.cpp $(TIMER_SRCS) $(COMMON_SRCS)

PROGS=	SDATtoNCSF/SDATtoNCSF SDATStrip/SDATStrip NDStoNCSF/NDStoNCSF 2SFTagsToNCSF/2SFTagsToNCSF
PROGS:=	$(sort $(PROGS))

PROG_SUFFIX=

COMPILER:=	$(shell $(CXX) -v 2>/dev/stdout)

MY_CPPFLAGS=	$(CPPFLAGS) -std=gnu++11 -I$(SRCDIR)common
MY_CXXFLAGS=	$(CXXFLAGS) -std=gnu++11 -pipe -Wall -Wctor-dtor-privacy -Wold-style-cast -Wextra -Wno-div-by-zero -Wfloat-equal -Wshadow -Winit-self -Wcast-qual -Wunreachable-code -Wabi -Woverloaded-virtual -Wno-long-long -Wno-switch -Wno-abi -I$(SRCDIR)common
ifeq (,$(findstring clang,$(COMPILER)))
MY_CXXFLAGS+=	-Wlogical-op
endif
MY_LDFLAGS=	$(LDFLAGS) -lz -pthread

ifneq (,$(findstring MINGW,$(UNAME)))
PROGS:=	$(addsuffix .exe,$(PROGS))
PROG_SUFFIX=	.exe
endif

PROG_SRCS_template=	$(1)_SRCS:=	$$(sort $$($(1)_SRCS))
PROG_OBJS_template=	$(1)_OBJS:=	$$(subst $(SRCDIR),,$$($(1)_SRCS:%.cpp=%.o))

$(foreach prog,$(PROGS),$(eval $(call PROG_SRCS_template,$(basename $(notdir $(prog))))))
$(foreach prog,$(PROGS),$(eval $(call PROG_OBJS_template,$(basename $(notdir $(prog))))))

SRCS:=	$(sort $(foreach prog,$(PROGS),$($(basename $(notdir $(prog)))_SRCS)))
OBJS:=	$(sort $(foreach prog,$(PROGS),$($(basename $(notdir $(prog)))_OBJS)))
DEPS:=	$(OBJS:%.o=%.d)

.PHONY: all debug clean

.SUFFIXES:
.SUFFIXES: .cpp .o .d $(PROG_SUFFIX)

all: $(PROGS)
debug: CXXFLAGS+=	-g -D_DEBUG
debug: all

define PROG_template
$(1): $$($$(basename $$(notdir $(1)))_OBJS)
	@echo "Linking $$@..."
	@$$(CXX) $$(MY_CXXFLAGS) -o $$@ $$^ $$(MY_LDFLAGS)
endef
define SRC_template
$$(subst $(SRCDIR),,$(1:%.cpp=%.o)): $(1)
	@echo "Compiling $$<..."
	@$$(CXX) $$(MY_CXXFLAGS) -o $$(subst $(SRCDIR),,$$@) -c $$<
endef
define DEP_template
$$(subst $(SRCDIR),,$(1:%.cpp=%.d)): $(1)
	@echo "Calculating depends for $$<..."
	-@mkdir -p $$(@D)
	@$$(CXX) $$(MY_CPPFLAGS) -MM -MF $$(subst $(SRCDIR),,$$@).tmp $$<
	@sed 's,$$(notdir $$*)\.o[ :]*,$$(subst /,\/,$$*).o $$(subst /,\/,$$@): ,g' < $$(subst $(SRCDIR),,$$@).tmp > $$(subst $(SRCDIR),,$$@)
	@rm $$(subst $(SRCDIR),,$$@).tmp
endef

$(foreach prog,$(PROGS),$(eval $(call PROG_template,$(prog))))
$(foreach src,$(SRCS),$(eval $(call SRC_template,$(src))))
$(foreach src,$(SRCS),$(eval $(call DEP_template,$(src))))

clean:
	@echo "Cleaning OBJs and PROGs..."
	-@rm $(OBJS) $(PROGS)

-include $(DEPS)
