.PHONY: all clean

# Based on https://github.com/yeputons/project-templates

TARGETS=test client server
SRCS_common=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/protocol.cpp
SRCS_test=$(SRCS_common) $(SRCDIR)/test.cpp $(SRCDIR)/test_protocol.cpp
SRCS_client=$(SRCS_common) $(SRCDIR)/client.cpp
SRCS_server=$(SRCS_common) $(SRCDIR)/server.cpp
BINDIR=.bin
SRCDIR=src
INCDIR=inc

all: $(TARGETS)
test: $(SRCS_test:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
client: $(SRCS_client:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)
server: $(SRCS_server:$(SRCDIR)/%.cpp=$(BINDIR)/%.o)

CXX=g++
CXXFLAGS=-pthread -pedantic -Wall -Wshadow -Wextra -Werror -std=c++11 -I$(INCDIR)
LDFLAGS=-pthread
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
