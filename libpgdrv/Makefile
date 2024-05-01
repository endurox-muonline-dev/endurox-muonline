# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=driver.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=libpgdrv.a

# Use this flags with compiler
CFLAGS=-O0 -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/include -I$(MU_MODULE_ROOT)/libpgdrv/include -I/usr/include/postgresql

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "========================"
	@echo "Creating static library $@"
	@echo "========================"
	ar -rc $@ $^

%.o: %.cpp
	@echo "$(CC) $(CFLAGS) -c $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@-rm -f $(TARGET) $(OBJECTS)