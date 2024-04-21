# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=connectserver_svc

# Use this flags with compiler
CFLAGS=-O0 -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/include
LIBS=-latmisrv -latmi -lubf -lnstd -lrt -ldl -ltinyxml2

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
