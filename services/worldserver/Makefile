# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=worldserver

# Use this flags with compiler
# For debug: -fsanitize=address -fno-omit-frame-pointer -g -O2
CFLAGS=-O0 -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/include -I$(MU_MODULE_ROOT)/libmucore/include -I$(MU_MODULE_ROOT)/libmucore -L$(MU_MODULE_ROOT)/libmucore
LIBS=-latmisrv -latmi -lubf -lnstd -lrt -ldl -ltinyxml2 -lmucore

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "========================"
	@echo "Linking $@"
	@echo "========================"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	@echo "========================"
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@-rm -f $(TARGET) $(OBJECTS)

install:
	@mkdir -p $(PREFIX)
	install $(TARGET) $(PREFIX)
