# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=main.cpp protocol/protocol.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=joinserver

# Use this flags with compiler
# For debug: -fsanitize=address -fno-omit-frame-pointer -g -O2
CFLAGS=-O0 -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/include -I$(MU_MODULE_ROOT)/services/joinserver -I$(MU_MODULE_ROOT)/libpgdrv/include -I$(MU_MODULE_ROOT)/libmucore/include -I$(MU_MODULE_ROOT)/libmucore -I/usr/include/postgresql -L$(MU_MODULE_ROOT)/libmucore -L$(MU_MODULE_ROOT)/libpgdrv -L/usr/lib/x86_64-linux-gnu
LIBS=-latmisrv -latmi -lubf -lnstd -lrt -ldl -ltinyxml2 -lpgdrv -lpq -lmucore -lcrypto

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
