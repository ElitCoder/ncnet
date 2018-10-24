NAME		:= libncnet

OBJ_FOLDER	:= obj
SRC_FOLDER	:= src
LIB_FOLDER	:= lib
INC_FOLDER	:= include

PREFIX		:= /usr
INST_INC	:= $(PREFIX)/include
INST_LIB	:= $(PREFIX)/lib

CPP_FILES	:= $(wildcard $(SRC_FOLDER)/*.cpp)
OBJ_FILES	:= $(addprefix $(OBJ_FOLDER)/,$(notdir $(CPP_FILES:.cpp=.o)))

CXX_FLAGS	:= -std=c++14 -Wall -Wextra -pedantic-errors
CXX_FLAGS	+= -g
#CXX_FLAGS	+= -O3
CXX_FLAGS	+= -fPIC
CXX_FLAGS	+= -I./$(INC_FOLDER)

LDLIBS		:= -lpthread
LIB_TYPE	:= -shared
LIB_NAME	:= $(NAME).so
TARGET		:= $(LIB_FOLDER)/$(LIB_NAME)

all: build

clean:
	rm -f $(LIB_FOLDER)/* $(OBJ_FOLDER)/*

install:
	mkdir -p $(INST_INC)/$(NAME)
	cp -r $(INC_FOLDER)/* $(INST_INC)/$(NAME)
	cp $(TARGET) $(INST_LIB)
	chmod 0755 $(INST_LIB)/$(LIB_NAME)
	chmod -R 0644 $(INST_INC)/$(NAME)/*

uninstall:
	rm -rf $(INST_INC)/$(NAME)
	rm -f $(INST_LIB)/$(LIB_NAME)

build: $(OBJ_FILES)
	$(CXX) $^ -o $(TARGET) $(LIB_TYPE) $(LD_LIBS)

$(OBJ_FOLDER)/%.o: $(SRC_FOLDER)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

CXX_FLAGS += -MMD
-include $(OBJFILES:.o=.d)