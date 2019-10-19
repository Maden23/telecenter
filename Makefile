CC = g++

TARGET   = recorder

SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS += -Iinclude 
CFLAGS += -Wall `pkg-config --cflags gstreamer-video-1.0 gstreamer-1.0 gtk+-3.0`
LDFLAGS += `pkg-config --libs gstreamer-video-1.0 gstreamer-1.0 gtk+-3.0`

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)				
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

