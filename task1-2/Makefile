.PHONY: all clean

# Based on https://github.com/yeputons/project-templates

TARGETS=bin/test bin/client bin/server
SRCS_common=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/protocol.cpp
SRCS_test=$(SRCS_common) $(SRCDIR)/test.cpp $(SRCDIR)/test_protocol.cpp
SRCS_client=$(SRCS_common) $(SRCDIR)/client.cpp
SRCS_server=$(SRCS_common) $(SRCDIR)/server.cpp
OBJDIR=.obj
SRCDIR=src
INCDIR=inc

all: $(TARGETS)
bin/test: $(SRCS_test:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
bin/client: $(SRCS_client:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
bin/server: $(SRCS_server:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

CXX=g++
CXXFLAGS=-pthread -pedantic -Wall -Wshadow -Wextra -Werror -std=c++11 -I$(INCDIR)
LDFLAGS=-pthread
LDLIBS=
ifeq ($(OS),Windows_NT)
  CXXFLAGS+=-DWINVER=WindowsXP
  LDLIBS+=-lws2_32
endif

$(TARGETS): | bin
	$(CXX) -o $@ $(LDFLAGS) $^ $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

bin:
	mkdir bin

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(TARGETS) $(OBJDIR) bin

include $(wildcard $(OBJDIR)/*.d)
