LIB				:= ncnet

SRCS			:= $(wildcard src/*.cpp)
OBJS			:= $(SRCS:.cpp=.o)

CXXFLAGS		+= -std=c++14 -Wall -Wextra -pedantic-errors
CXXFLAGS		+= -g
CXXFLAGS		+= -fPIC
CXXFLAGS		+= -fopenmp

LDLIBS			+= -lpthread -fopenmp

PREFIX			:= /usr
INSTALL_INCLUDE	:= $(PREFIX)/include/$(LIB)
INSTALL_LIB		:= $(PREFIX)/lib

# Include headers
CXXFLAGS		+= -Iinclude

# Build both shared and static libraries
TARGET_STATIC	:= lib/lib$(LIB).a
TARGET_SHARED	:= lib/lib$(LIB).so

# Test
TEST_SRCS		:= $(wildcard test/*.cpp)
TEST_OBJS		:= $(TEST_SRCS:.cpp=.o)
TEST_TARGET		:= test/test_ncnet
TEST_LDFLAGS	:= -Llib
TEST_LDLIBS		:= -l:libncnet.a

.PHONY: all
all: $(TARGET_SHARED) $(TARGET_STATIC)

.PHONY: test
test: all $(TEST_TARGET)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET_SHARED) $(TARGET_STATIC) $(OBJS:.o=.d)
	rm -f $(TEST_OBJS) $(TEST_TARGET) $(TEST_OBJS:.o=.d)

.PHONY: install
install:
	cp $(TARGET_SHARED) $(TARGET_STATIC) $(INSTALL_LIB)
	mkdir -p $(INSTALL_INCLUDE)
	cp include/*.h $(INSTALL_INCLUDE)

$(TARGET_SHARED): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET_SHARED) -shared $^ $(LDLIBS)

$(TARGET_STATIC): $(OBJS)
	ar -rs $(TARGET_STATIC) $(OBJS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $^ $(LDFLAGS) $(TEST_LDFLAGS) -o $(TEST_TARGET) $(LDLIBS) $(TEST_LDLIBS)

CXXFLAGS		+= -MMD
-include $(OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)