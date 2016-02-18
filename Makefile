 # $Id: Makefile 227 2010-11-25 14:30:08Z krasznaa $
 ###########################################################################
 # @Project: SFrame - ROOT-based analysis framework for ATLAS              #
 # @Package: Core                                                          #
 #                                                                         #
 # @author Stefan Ask       <Stefan.Ask@cern.ch>           - Manchester    #
 # @author David Berge      <David.Berge@cern.ch>          - CERN          #
 # @author Johannes Haller  <Johannes.Haller@cern.ch>      - Hamburg       #
 # @author A. Krasznahorkay <Attila.Krasznahorkay@cern.ch> - CERN/Debrecen #
 #                                                                         #
 # Makefile compiling the SFrameCore library.                              #
 #                                                                         #
 ###########################################################################

# Package information
APP = manager
LIBRARY = manager
OBJDIR  = obj
DEPDIR  = $(OBJDIR)/dep
SRCDIR  = src
INCDIR  = interface

ifeq ($(shell root-config --platform), macosx)
	BOOSTLIBS = -lboost_system-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_regex-mt
else
	BOOSTLIBS = -L/afs/cern.ch/cms/slc6_amd64_gcc472/external/boost/1.51.0-cms/lib/ -lboost_filesystem -lboost_program_options -lboost_regex -lboost_system
	BOOST_INC = -I/afs/cern.ch/cms/slc6_amd64_gcc472/external/boost/1.51.0-cms/include/
	# BOOSTLIBS = -L/afs/cern.ch/cms/slc6_amd64_gcc434/external/boost/1.44.0-cms/lib/ -lboost_filesystem -lboost_program_options -lboost_regex -lboost_system
	# BOOST_INC = -I/afs/cern.ch/cms/slc6_amd64_gcc434/external/boost/1.44.0-cms/include/

	INCLUDES += $(BOOST_INC) 
endif

CXXFLAGS += -Wall -Wno-overloaded-virtual -Wno-unused -std=c++11

MOREROOTLIBS = -lRooFit -lRooFitCore -lRooStats -lHistFactory -lMinuit -lMathMore -lSmatrix


# Overwrite the default rule defined in Makefile.common
coredefault: default $(_BIN_PATH)/$(APP)

# Include the library compilation rules
include $(_DIR)/Makefile.common

#
# Rules for compiling the executable
#
# Reminder: $(ROOTLIBS) comes from Makefile.arch which is picked up from the ROOT
# sources by Makefile.common...
#

$(_BIN_PATH)/$(APP): $(APP).o $(SHLIBFILE)
	@echo "Linking " $@
	@$(LD) $(LDFLAGS) -O2 $(OBJDIR)/$(APP).o -L$(_LIB_PATH) -l$(LIBRARY) \
		$(ROOTLIBS) $(MOREROOTLIBS) $(BOOSTLIBS) -o $@

$(APP).o: app/$(APP).cxx 
	@echo "Compiling $<"
	@mkdir -p $(OBJDIR)
	@$(CXX) $(CXXFLAGS) -O2 -c $< -o $(OBJDIR)/$(notdir $@) $(INCLUDES)
