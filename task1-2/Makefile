.PHONY: all clean

# Based on https://github.com/yeputons/project-templates

TARGETS=test client server
SRCS_common=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/protocol.cpp
SRCS_test=$(SRCS_common) $(SRCDIR)/test.cpp $(SRCDIR)/test_protocol.cpp
SRCS_client=$(SRCS_common) $(SRCDIR)/client.cpp
SRCS_server=$(SRCS_common) $(SRCDIR)/server.cpp
OBJDIR=.obj
SRCDIR=src
INCDIR=inc

all: $(TARGETS)
test: $(SRCS_test:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
client: $(SRCS_client:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
server: $(SRCS_server:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

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

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(TARGETS) $(OBJDIR)

include $(wildcard $(OBJDIR)/*.d)
