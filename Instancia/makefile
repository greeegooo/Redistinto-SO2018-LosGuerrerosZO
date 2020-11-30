#********************************************************************
#	                         MAKEFILE                               *
#********************************************************************

#********************************************************************
#	 Author: gmichalopulos                                          *
#   Version: v2.0                                                   *
#      Date: wed 25-04-2018 19:40                                   *
#********************************************************************

#**********************************************************SUMMARY***
#                                                                   *
#  - The objective of this makefile is to automatize the tasks      *
#    related to: BUILD, RUN, TEST and CLEAN                         *
#                                                                   *
#  - This makefile works for the following folder structure         *
#                                                                   *                                        
#	 PROJECT_FOLDER                                                 *
#      |_includes/..      *.h files                                 *
#      |_src/..           *.c files                                 *
#      |_tests/..         *.c files for testing purposes            *
#      |_main.c           main file of the project                  *
#      |_progExec         .exe generated for program                *
#      |_testExec         .exe generated for test                   *
#      |_makefile         this makefile                             *                              
#                                                                   *
#  - For more info:                                                 *         
#    https://www.gnu.org/software/make/manual/html_node/            *
#    https://gcc.gnu.org/onlinedocs/gcc/index.html                  *
#    https://www.gnu.org/software/libc/manual/html_mono/libc.html   *      
#                                                                   *
#********************************************************************

#********************************************************************
#																	*
#                              MACROS                               *
#																	*
#********************************************************************

#*********************************************************COMPILER***
# 																	*
# Replace here to choose your compiler                              *
#********************************************************************
CC = gcc

#************************************************************FLAGS***
#																	*
# Add your compiler options											*
#********************************************************************
CFGLAGS = -Wall -g

#******************************************************DIRECTORIES***
#																	*
# Add here the directories for your project							*
#********************************************************************
SRCDIR = src
INCDIR = includes
TSTDIR = tests
UTIDIR = ../Utils
UTIDIR_H = ../Utils/includes
#********************************************************LIBRARIES***
#																	*
# Add here the libraries you want to link 							*
#********************************************************************
LIBS = -lcspecs -lcommons -lreadline -lcrypto -pthread -lm

#**********************************************************SOURCES***
#																	*
# References to the .c files 							            *
#********************************************************************
MAIN = main.c

TEST = tests/tests.c

_SRC = *.c
SRC = $(patsubst %,$(SRCDIR)/%,$(_SRC))

_UTI = *.c
UTI = $(patsubst %,$(UTIDIR)/%,$(_UTI))

_TST = *.c
TST = $(patsubst %,$(TSTDIR)/%,$(_TST))

#*********************************************************INCLUDES***
#																	*
# References to the .h files 							            *
#********************************************************************
_INC = *.h
INC = $(patsubst %,$(INCDIR)/%,$(_INC))

_UTI_H = *.h
UTI_H = $(patsubst %,$(UTIDIR_H)/%,$(_UTI_H))
#******************************************************EXECUTABLES***
#																	*
# Replace here to change your exec names							*
#********************************************************************
PROGNAME = instancia
TESTNAME = test

#********************************************************************
#																	*
#                              RULES                                *
#     																*
#********************************************************************

#**************************************************************ALL***
#																	*
# Default command. 													*
# For more details see the rules explanations.						*
#********************************************************************
all: clean install test run
	
#**********************************************************INSTALL***
#																	*
# Generates the executables for prog and test						*
#********************************************************************
.PHONY: install
install: installProg #installTest

#*****************************************************INSTALL_PROG***
#																	*
# Generate the executable for prog									*
#********************************************************************
installProg:
	@echo --- Building $(PROGNAME) ---
	$(CC) -o $(PROGNAME) $(MAIN) $(SRC) $(UTI) $(INC) $(UTI_H) $(CFGLAGS) $(LIBS)
	@echo --- Builded $(PROGNAME) ---

#*****************************************************INSTALL_TEST***
#																	*
# Generate the executable for tests									*
#********************************************************************
installTest:
	@echo --- Building $(TESTNAME) ---
	$(CC) -o $(TESTNAME) $(TEST) $(SRC) $(UTI) $(INC) $(UTI_H) $(CFGLAGS) $(LIBS)
	@echo --- Builded $(TESTNAME) ---

#**************************************************************RUN***
#																	*
# Run the prog														*
#********************************************************************
.PHONY: run
run: 
	@echo Running $(PROGNAME)
	./$(PROGNAME) $(CONFIG)

#*************************************************************TEST***
#																	*
# Run the tests														*
#********************************************************************
.PHONY:test
test: 
	@echo Running $(TESTNAME)
	./$(TESTNAME)

#************************************************************CLEAN***
#																	*
# Remove the executables from current scope directory               *
#********************************************************************
.PHONY: clean
clean: cleanProg cleanTest
# Descomentar para borrar tambien el log
#	rm instancia.log

#*******************************************************CLEAN_PROG***
#																	*
#																	*
# Remove the prog executable from current scope directory           *
#********************************************************************
cleanProg:
	@echo Deleting $(PROGNAME)
	rm -f $(PROGNAME)
	@echo Program deleted
	
#*******************************************************CLEAN_TEST***
#																	*
# Remove the test executable from current scope directory           *
#********************************************************************
cleanTest:
	@echo Deleting $(TESTNAME)
	rm -f $(TESTNAME)
	@echo Tests deleted

#********************************************************************
#********************************************************************