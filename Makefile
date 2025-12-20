TOPDIR		?=	$(CURDIR)

TARGET		:=	fire

BUILD_ROOT		:=	build
BUILD_DEBUG		:=	debug
BUILD_RELEASE	:=	release
#BUILD			?=

BACKUP_FOLDER	:=	$(TOPDIR)/.bak

INCLUDE_DIR		:=	include

SOURCE_DIR		:=	src \
					src/Parser \
					src/Sema

CC			:=	clang
CXX			:=	clang++

EXT_ASM		:=	.s
EXT_C		:=	.c
EXT_CXX		:=	.cpp

FLAGS_OPTIMIZE	:=	-O3
FLAGS_WARN		:=	-Wall -Wextra -Wno-switch
FLAGS_DEFINE	:=

COMMONFLAGS		:=	\
	$(INCLUDES) \
	$(FLAGS_OPTIMIZE) \
	$(FLAGS_WARN) \
	$(foreach d,$(FLAGS_DEFINE),-D$(d))

FLAGS_C			:=	$(COMMONFLAGS) -std=c17
FLAGS_CXX		:=	$(COMMONFLAGS) -std=c++20
FLAGS_LD		:=	-Wl,--gc-sections,-s -fuse=mold

%.o: %$(EXT_ASM)
	@echo "\e[1m\e[32mCOMPILE \e[37m$<\e[0m ..."
	@$(CC) -MP -MMD -MF $@.d -c -o $@ $<

%.o: %$(EXT_C)
	@echo "\e[1m\e[32mCOMPILE \e[37m$<\e[0m ..."
	@$(CC) -MP -MMD -MF $@.d $(FLAGS_C) -c -o $@ $<

%.o: %$(EXT_CXX)
	@echo "\e[1m\e[32mCOMPILE \e[37m$<\e[0m ..."
	@$(CXX) -MP -MMD -MF $@.d $(FLAGS_CXX) -c -o $@ $<

ifneq ($(notdir $(CURDIR)), $(BUILD))

export	TOPDIR	:=	$(CURDIR)
export	OUTPUT	:=	$(TOPDIR)/$(TARGET)
export	VPATH	:=	$(foreach dir,$(SOURCE_DIR),$(TOPDIR)/$(dir))

export	INCLUDES	:=	$(foreach dir,$(INCLUDE_DIR),-I$(TOPDIR)/$(dir))

ASMFILES	:=	$(notdir $(foreach dir,$(SOURCE_DIR),$(wildcard $(dir)/*$(EXT_ASM))))
CFILES		:=	$(notdir $(foreach dir,$(SOURCE_DIR),$(wildcard $(dir)/*$(EXT_C))))
CXXFILES	:=	$(notdir $(foreach dir,$(SOURCE_DIR),$(wildcard $(dir)/*$(EXT_CXX))))

export	OFILES	:=	$(CXXFILES:$(EXT_CXX)=.o)

.PHONY: all debug release clean re bak bak_full restore

all: debug

debug:
	@mkdir -p $(BUILD_DEBUG)
	@$(MAKE) --no-print-directory \
		FLAGS_OPTIMIZE="-O0 -g" \
		FLAGS_LD="" \
		FLAGS_DEFINE="_FIRE_DEBUG_" \
		BUILD=$(BUILD_DEBUG) \
		BUILDMODE=$(BUILD_DEBUG) \
		-C $(BUILD_DEBUG) -f $(TOPDIR)/Makefile

release:
	@mkdir -p $(BUILD_RELEASE)
	@$(MAKE) --no-print-directory \
		BUILD=$(BUILD_RELEASE) \
		BUILDMODE=$(BUILD_RELEASE) \
		-C $(BUILD_RELEASE) -f $(TOPDIR)/Makefile

clean: bak
	@echo "\e[31mCleaning\e[0m"
	rm -rf $(BUILD_DEBUG) $(BUILD_RELEASE) $(TARGET)

re: clean all

bak:
	@mkdir -p $(BACKUP_FOLDER)
	@rm -rf $(BACKUP_FOLDER)/*
	@if [ -d $(BUILD_DEBUG) ]; then cp -r $(BUILD_DEBUG) $(BACKUP_FOLDER); fi
	@if [ -d $(BUILD_RELEASE) ]; then cp -r $(BUILD_RELEASE) $(BACKUP_FOLDER); fi
	@if [ -f $(TARGET) ]; then cp $(TARGET) $(BACKUP_FOLDER); fi
	@echo "\e[1mCreated backup into: $(BACKUP_FOLDER)\e[0m"

rmbak:
	rm -rf .bak

restore:
	@if [ -d $(BACKUP_FOLDER) ]; then \
		cp $(BACKUP_FOLDER)/* ./ \
	else \
		echo backup folder not found. \
	fi

else

DEPENDS		:=	$(OFILES:.o=.o.d)

CMP1	:=	$(shell echo $(notdir $(wildcard $(CURDIR)/*.o)) | tr ' ' '\n' | sort | paste -sd' ' -)
CMP2	:=	$(shell echo $(OFILES) | tr ' ' '\n' | sort | paste -sd' ' -)

ifneq ($(CMP1),$(CMP2))
STARTMSG	:=	_start
.PHONY: $(STARTMSG)
endif

$(OUTPUT): $(STARTMSG) $(OFILES)
	@echo "\e[36mLINKING TO \e[37m$@ \e[0m..."
	@$(CXX) $(LDFLAGS) -o $@ $(OFILES)
	@if [ "$(STARTMSG)" != "" ]; then echo "\n\e[1m--- Done. ---\e[0m"; fi

ifneq ($(CMP1),$(CMP2))
$(STARTMSG):
	@echo "\e[1m--- Start building for \e[35m$(BUILDMODE)\e[37m ---\e[0m"
endif

-include $(DEPENDS)

endif

