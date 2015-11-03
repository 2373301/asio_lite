
###############################################################################
CC = g++
SRC_DIR = ./
INC_DIR = -I. 
LIB_DIR = -L./
LIBS = -lpthread 
DEST = libutil.a
OUTPUT = build
###############################################################################

SRCS = $(shell find $(SRC_DIR) -maxdepth 1 -name "*.cpp" )
OBJS = $(patsubst %.cpp, $(OUTPUT)/%.o, $(SRCS))
DEPS = $(patsubst %.o, %.d, $(OBJS))

$(foreach dir, $(SRC_DIR), $(eval OBJ_DIR += $(OUTPUT)/$(dir)))

CPPFLAGS = -g -O2 $(INC_DIR) -Wall 
LDFLAGS = -g -O2 $(LIB_DIR) $(LIBS)

all: $(DEST)
$(shell mkdir -p $(sort $(OBJ_DIR)))
include $(DEPS)

$(DEST): $(OBJS)
	ar -r $(DEST) $(OBJS)
#	$(CC) -o $@ $^ $(LDFLAGS) #-shared

$(OUTPUT)/%.o: %.cpp
	$(CC) -o $@ -c -fPIC $< $(CPPFLAGS)

$(OUTPUT)/%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OUTPUT)/%.o: %.c
	$(CC) -o $@ -c -fPIC $< $(CPPFLAGS)

$(OUTPUT)/%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OUTPUT)
	rm -f $(DEST)
