CXX=g++
CXX_FLAGS=-g -Og -std=c++20 -march=native -Wall -Wextra -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wundef -Werror -Wno-unused

LD=g++
LD_FLAGS=-g

SRC_DIR=src
LIB_DIR=lib

BUILD_DIR=obj
OUT_DIR=dist
OUT_NAME=obd2-server
STATIC_DIR=static

USER=pi

LIB_INCLUDES=$(foreach include,$(shell find $(LIB_DIR) -type d -name 'include'),-I$(include) )

CXX_SOURCES:=$(shell find $(SRC_DIR) -name '*.cpp') $(shell find $(LIB_DIR) -name '*.cpp')
OBJECTS:=$(addprefix $(BUILD_DIR)/,$(CXX_SOURCES:.cpp=.o))

$(OUT_DIR)/$(OUT_NAME): $(OBJECTS)
	mkdir -p $(dir $@)
	$(LD) -o $@ $(LD_FLAGS) $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(LIB_INCLUDES) -c $< -o $@ $(CXX_FLAGS)

install: $(OUT_DIR)/$(OUT_NAME) install_service
	systemctl stop $(OUT_NAME)

	cp $(OUT_DIR)/$(OUT_NAME) /usr/bin/$(OUT_NAME)

	mkdir -p /home/$(USER)/.config
	cp -r $(STATIC_DIR)/config/* /home/$(USER)/.config/$(OUT_NAME)

	systemctl start $(OUT_NAME)

install_service:
	cp $(STATIC_DIR)/obd2-server.service /etc/systemd/system/$(OUT_NAME).service
	sed -i 's|<OUT_NAME>|$(OUT_NAME)|g' /etc/systemd/system/$(OUT_NAME).service
	sed -i 's|<USER>|$(USER)|g' /etc/systemd/system/$(OUT_NAME).service

	systemctl enable $(OUT_NAME)

clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)

.PHONY: clean