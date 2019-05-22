# makefile for LCH unit tests

###############################################################################
## Copyright 2018-2019 by Joyz Inc of Tokyo, Japan (author: Charles Hussong) ##
##                                                                           ##
## Licensed under the Apache License, Version 2.0 (the "License");           ##
## you may not use this file except in compliance with the License.          ##
## You may obtain a copy of the License at                                   ##
##                                                                           ##
##    http://www.apache.org/licenses/LICENSE-2.0                             ##
##                                                                           ##
## Unless required by applicable law or agreed to in writing, software       ##
## distributed under the License is distributed on an "AS IS" BASIS,         ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  ##
## See the License for the specific language governing permissions and       ##
## limitations under the License.                                            ##
###############################################################################


#-------------------------------------------------------------------------------
# setup, declarations, options
#-------------------------------------------------------------------------------

# definitions of various targets
TEST_EXEC := lch_test
INCDIR := ../include
SRCDIR := .
OBJDIR := objects
INSTALL_DEST := /usr/local/include/LCH

CFLAGS :=

CXXFLAGS := -I$(INCDIR) -Werror -Wall -Wextra -pedantic -fPIC -g -std=c++17

LDFLAGS := -lm -lpthread

SOURCES := $(wildcard $(SRCDIR)/*.cpp)

DEPFILES := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)

OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

HEADERS := $(wildcard $(INCDIR)/*.hpp)

#-------------------------------------------------------------------------------
# meta targets
#-------------------------------------------------------------------------------

.PHONY: all, clean, install

all: $(TEST_EXEC)

clean:
	rm -f $(TEST_EXEC) $(DEPFILES) $(OBJECTS)

install: | $(INSTALL_DEST)
	install -m 644 $(HEADERS) $(INSTALL_DEST)

$(INSTALL_DEST):
	mkdir $@

#-------------------------------------------------------------------------------
# final targets
#-------------------------------------------------------------------------------

$(TEST_EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

#-------------------------------------------------------------------------------
# intermediate dependency and object targets
#-------------------------------------------------------------------------------

# build dependency file first, then build object file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) -MM -MP -MT $@ -MT $(OBJDIR)/$*.d $(CXXFLAGS) $< > $(OBJDIR)/$*.d
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(OBJDIR):
	mkdir $@

#-------------------------------------------------------------------------------
# include the auto-generated dependency files at the end
#-------------------------------------------------------------------------------

-include $(DEPFILES)

