# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=simple_modulus/simple_modulus.cpp thread_manager/thread_manager.cpp utils.cpp reply_manager/reply_manager.cpp config/config.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=libmucore.a

# Use this flags with compiler
CFLAGS=-O0 -g -I$(MU_MODULE_ROOT)/include -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/libmucore/include
LIBS=-latmi -lubf -lnstd -lrt -ldl -ltinyxml2 -lcrypto

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "========================"
	@echo "Creating static '$(TARGET)' library"
	@echo "========================"
	ar -rc $@ $^

%.o: %.cpp
	@echo "$(CC) $(CFLAGS) -c $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@-rm -f $(TARGET) $(OBJECTS)