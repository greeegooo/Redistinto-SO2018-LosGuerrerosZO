#********************************************************************
#	                         MAKEFILE                               *
#********************************************************************

#********************************************************************
#	 Author: gmichalopulos                                          *
#   Version: v2.0                                                   *
#      Date: wed 29-04-2018 00:45                                   *
#********************************************************************

#**********************************************************SUMMARY***
#                                                                   *
#  - The objective of this makefile is to automatize the tasks      *
#    related to: BUILD, RUN, TEST and CLEAN                         *
#                                                                   *
#  - This makefile works for the following folder structure         *
#                                                                   *
#	 SYSTEM_FOLDER                                                  *
#      |_Proj_1/..        project 1 folder                          *
#      |_Proj_2/..        											*
#      |_.                											*
#      |_.                											*
#      |_Proj_N/..        											*
#      |_Utils/..         common files for all projects             *
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

#********************************************************************
#																	*
#                               RULES                               *
#																	*
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
install: installCoordinador installInstancia installPlanificador installESI
	
installCoordinador:
	cd Coordinador/ && $(MAKE) install

installInstancia:
	cd Instancia/ && $(MAKE) install

installPlanificador:
	cd Planificador/ && $(MAKE) install

installESI:
	cd ESI/ && $(MAKE) install
#**************************************************************RUN***
#																	*
# Run the prog														*
#********************************************************************
run: runCoordinador runInstancia runPlanificador runESI
	@figlet -cf term Los Guerreros ZO
	@figlet -cf term Bienvenido a
	@figlet -ck ReDistinto

runCoordinador:
	cd Coordinador/ && $(MAKE) run

runInstancia:
	cd Instancia/ && $(MAKE) run

runPlanificador:
	cd Planificador/ && $(MAKE) run

runESI:
	cd ESI/ && $(MAKE) run

#************************************************************CLEAN***
#																	*
# Remove the executables from current scope directory               *
#********************************************************************
clean: cleanCoordinador cleanInstancia cleanPlanificador cleanESI

cleanCoordinador:
	cd Coordinador/ && $(MAKE) clean
cleanInstancia:
	cd Instancia/ && $(MAKE) clean
cleanPlanificador:
	cd Planificador/ && $(MAKE) clean
cleanESI:
	cd ESI/ && $(MAKE) clean

#*************************************************************TEST***
#																	*
# Run the tests														*
#********************************************************************
test: testCoordinador testInstancia testPlanificador testESI

testCoordinador:
	cd Coordinador/ && $(MAKE) test
testInstancia:
	cd Instancia/ && $(MAKE) test
testPlanificador:
	cd Planificador/ && $(MAKE) test
testESI:
	cd ESI/ && $(MAKE) test

#********************************************************************
#********************************************************************