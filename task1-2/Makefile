.PHONY: all clean

# Based on https://github.com/yeputons/project-templates

TARGETS=test
SRCS_test=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/test.cpp
BINDIR=.bin
SRCDIR=src
INCDIR=inc

all: $(TARGETS)
test: $(SRCS_test:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)

CXX=g++
CXXFLAGS=-pedantic -Wall -Wshadow -Wextra -Werror -std=c++11 -I$(INCDIR)
LDFLAGS=
LDLIBS=
ifeq ($(OS),Windows_NT)
  CXXFLAGS+=-DWINVER=WindowsXP
  LDLIBS+=-lws2_32
endif

$(TARGETS):
	$(CXX) -o $@ $(LDFLAGS) $^ $(LDLIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BINDIR):
	mkdir $(BINDIR)

clean:
	rm -rf $(TARGETS) .bin

include $(wildcard $(BINDIR)/*.d)
