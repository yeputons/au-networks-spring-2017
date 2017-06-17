.PHONY: all all32 clean

# Based on https://github.com/yeputons/project-templates

TARGETS=bin/test32 bin/test64 bin/client32 bin/client64 bin/server32 bin/server
SRCS_common=$(SRCDIR)/tcp_socket.cpp $(SRCDIR)/au_stream_socket.cpp $(SRCDIR)/au_stream_socket_impl.cpp $(SRCDIR)/au_stream_socket_protocol.cpp $(SRCDIR)/common_socket_impl.cpp $(SRCDIR)/protocol.cpp
SRCS_test=$(SRCS_common) $(SRCDIR)/test.cpp $(SRCDIR)/test_protocol.cpp
SRCS_client=$(SRCS_common) $(SRCDIR)/client.cpp
SRCS_server=$(SRCS_common) $(SRCDIR)/server.cpp
OBJDIR=.obj
SRCDIR=src
INCDIR=inc

all: $(TARGETS)
all32: $(filter %32, $(TARGETS))
bin/test32: $(SRCS_test:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o32)
bin/test64: $(SRCS_test:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o64)
bin/client32: $(SRCS_client:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o32)
bin/client64: $(SRCS_client:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o64)
bin/server32: $(SRCS_server:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o32)
bin/server: $(SRCS_server:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o64)

CXX=g++
CXXFLAGS=-pthread -pedantic -Wall -Wshadow -Wextra -Werror -std=gnu++11 -I$(INCDIR) -g
LDFLAGS=-pthread
LDLIBS=
LDLIBS_TEST=-lgtest -lgtest_main
ifeq ($(OS),Windows_NT)
  CXXFLAGS+=-DWINVER=WindowsXP
  LDLIBS+=-lws2_32
endif

bin/test32: | bin
	$(CXX) -m32 -o $@ $(LDFLAGS) $^ $(LDLIBS) $(LDLIBS_TEST)

bin/test64: | bin
	$(CXX) -m64 -o $@ $(LDFLAGS) $^ $(LDLIBS) $(LDLIBS_TEST)

bin/%32: | bin
	$(CXX) -m32 -o $@ $(LDFLAGS) $^ $(LDLIBS)

bin/%64: | bin
	$(CXX) -m64 -o $@ $(LDFLAGS) $^ $(LDLIBS)

bin/server: | bin
	$(CXX) -m64 -o $@ $(LDFLAGS) $^ $(LDLIBS)

$(OBJDIR)/%.o32: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) -m32 $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(OBJDIR)/%.o64: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) -m64 $(CXXFLAGS) -MMD -MP -c -o $@ $<

bin:
	mkdir bin

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(TARGETS) $(OBJDIR) bin

include $(wildcard $(OBJDIR)/*.d)
