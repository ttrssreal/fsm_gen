CXX = clang++
CXXFLAGS = -Wall -g
TARGET = fsm_gen
SRCS = \
	src/fsm_gen.cpp \
	src/fsm_parse.cpp \
	src/fsm_lexer.cpp \
	src/fsm_codegen.cpp
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

INCLUDES = -Isrc
CXXFLAGS += $(INCLUDES)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

$(OBJS): %.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
