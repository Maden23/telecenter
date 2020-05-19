# https://riptutorial.com/makefile/example/21376/building-from-different-source-folders-to-different-target-folders

# Название исполняемого файла
TARGET   = singlestream
# Директории, где лежат файлы
DIRS = . ../core
# Названия поддиректорий для исходного кода / заголовочных файлов / бинарнах файлов
SOURCEDIR = src
INCLUDEDIR = include
BUILDDIR = obj

# ------------------------------------------------------------------

# Создание списка директорий для исходного кода 
SOURCEDIRS = $(foreach dir, $(DIRS), $(addsuffix /$(SOURCEDIR), $(dir)))
INCLUDEDIRS = $(foreach dir, $(DIRS), $(addsuffix /$(INCLUDEDIR), $(dir)))
TARGETDIRS = $(foreach dir, $(DIRS), $(addsuffix /$(BUILDDIR), $(dir))))
# Дабавляем в VPATH, чтобы файлы искались во всех директориях
VPATH = $(SOURCEDIRS)



# Создание списка включений (тоже должны быть в src)
INCLUDES = $(foreach dir, $(INCLUDEDIRS), $(addprefix -I, $(dir)))
SOURCES = $(foreach dir, $(SOURCEDIRS),$(wildcard $(dir)/*.cpp))
OBJS = $(subst $(SOURCEDIR),$(BUILDDIR),$(SOURCES:.cpp=.o))


CFLAGS = -Wall 
# Подключение библиотек
CFLAGS += `pkg-config --cflags gstreamer-video-1.0 gstreamer-1.0 gtk+-3.0`
LDFLAGS = `pkg-config --libs gstreamer-video-1.0 gstreamer-1.0 gtk+-3.0`
LDFLAGS += -lpaho-mqtt3a -lpaho-mqttpp3

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -g -o $@ $(LDFLAGS)

MKDIR_P = mkdir -p
# Функция для создания правил компиляции для заданной директории
define generateRules
$(1)/$(BUILDDIR)/%.o: $(1)/$(SOURCEDIR)/%.cpp
	$(MKDIR_P) $(1)/$(BUILDDIR)
	$(CXX) $(INCLUDES) $(CFLAGS) -c $$< -g -o $$@
endef

# Создание правил для директорий, перечисленных в DIR 
$(foreach dir, $(DIRS), $(eval $(call generateRules, $(dir))))

# Удаление бинарных файлов
clean:
	$(RM) $(OBJS)