#
# hmtp Makefile
#

# Project files and folders
SRC_F = .
BIN_F = $(HMTP_CMP_PATH)
BINFN = hmtp

# PC compiler options
PC_CC 		= gcc
PC_LIBMTP = -lmtp
PC_BIN_F 	= ../../compiled/hmtp-pc

# Compiler Options GWC
CC 				= $(CROSS)gcc
C_FLAGS 	= $(CFLAGS) -s -std=c11 -DLINUX_TIME_H
LD_FLAGS	= $(LDFLAGS)
LIBMTP 		= -I$(LIBMTP_LINK_PATH) -L$(LIBMTP_SO_PATH) -lmtp -Wl,-rpath=$(LIBMTP_RUN_PATH)


# Files passed to compiler
COMPILE_OBJECTS = $(SRC_F)/hmtp.c $(SRC_F)/ffutils.c

# Declaring all targets Override problem with folder named equal to a target ( target gwc and folder gwc)
.PHONY: gwc pc help clean clean-gwc clean-pc

# Compiles hmtp for gwc
gwc:
	@make clean-gwc
	@echo "»»»"
	@echo "»»» COMPILING TO $(BIN_F)/$(BINFN) "
	@echo "»»»"
	@if [ ! -d $(BIN_F) ]; then mkdir $(BIN_F); fi
	$(CC) -o $(BIN_F)/$(BINFN) $(COMPILE_OBJECTS) $(C_FLAGS) $(LD_FLAGS) $(LIBMTP)

# Compiles hmtp for PC
pc:
	@make clean-pc
	@echo "»»»"
	@echo "»»» COMPILING TO $(PC_BIN_F)/$(BINFN) "
	@echo "»»»"
	@if [ ! -d $(PC_BIN_F) ]; then mkdir $(PC_BIN_F); fi
	$(PC_CC) -o $(PC_BIN_F)/$(BINFN) $(COMPILE_OBJECTS) $(PC_LIBMTP)

# Print a small help about makefile
help:
	@echo "────────────────────────────────────────────────────────────────────────────────────────────────────────────────"
	@echo "Project hmtp"
	@echo "Simple utility to navigate mtp share and get/send files through it"
	@echo ""
	@echo "Available make targets are:"
	@echo " » help      	Prints this help"
	@echo " » gwc       	Compiles source files for GWC ( needs env variables to be set )"
	@echo " » pc       		Compiles source files for PC"
	@echo " » clean     	Cleans everything under BIN folder"
	@echo " » clean-gwc   Cleans BIN executable"
	@echo " » clean-pc   	Cleans PC compiled folder"
	@echo ""
	@echo "Environmental variables:"
	@echo " » CROSS								compiler path/prefix ( /..../arm-unknown-linux-gnueabihf/bin/arm-unknown-linux-gnueabihf- )"
	@echo " » CCFLAGS							Optional CCFLAGS"
	@echo " » LDFLAGS							Optional LDFLAGS"
	@echo " » LIBMTP_LINK_PATH		Path of libmtp.h at linking time"
	@echo " » LIBMTP_SO_PATH			Path of libmtp.so at linktime"
	@echo " » LIBMTP_RUN_PATH			Path of libmtp.so at runtime"
	@echo ""


# Clean everything under BIN folder
clean: clean-gwc clean-pc

# Clean gwc executable
clean-gwc:
	@echo "»»»"
	@echo "»»» CLEANING BIN EXE: $(BIN_F)/$(BINFN) "
	@echo "»»»"
	@if [ -f $(BIN_F)/$(BINFN) ]; then rm -f $(BIN_F)/$(BINFN); fi

# Clean pc executable
clean-pc:
	@echo "»»»"
	@echo "»»» CLEANING PC FOLDER: $(PC_BIN_F)/$(BINFN) "
	@echo "»»»"
	@if [ -d $(PC_BIN_F) ]; then rm -f $(PC_BIN_F)/*; fi