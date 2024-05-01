# Use g++ compiler
CC=g++

# Source files goes here
SOURCES=main.cpp protocol/protocol.cpp jsprotocol/jsprotocol.cpp dsprotocol/dsprotocol.cpp client_info/client_info.cpp
OBJECTS=$(SOURCES:.cpp=.o)
TARGET=mainserver

# Use this flags with compiler
# For debug: -fsanitize=address -fno-omit-frame-pointer -g -O2
CFLAGS=-O0 -g -I$(MU_MODULE_ROOT)/ubftab/include -I$(MU_MODULE_ROOT)/include -I$(MU_MODULE_ROOT)/services/mainserver -I$(MU_MODULE_ROOT)/libmucore/include -I$(MU_MODULE_ROOT)/libmucore -L$(BASE_DIR)/libmucore
LIBS=-latmisrv -latmi -lubf -lnstd -lrt -ldl -ltinyxml2 -lmucore -lcrypto

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
