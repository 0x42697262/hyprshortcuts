TARGET = hyprshortcuts.so

# Pure, Hyprland-free layers — compiled into both the plugin and the test binary.
DOMAIN_SRC = src/domain/KeySymbols.cpp \
             src/domain/BindModel.cpp \
             src/domain/Grouping.cpp \
             src/domain/Layout.cpp \
             src/commands/CommandRegistry.cpp

# Hyprland-coupled layers — plugin only.
PLUGIN_SRC = src/main.cpp \
             src/OverlayController.cpp \
             src/BindSource.cpp \
             src/render/TextRenderer.cpp \
             src/render/OverlayRenderer.cpp \
             $(DOMAIN_SRC)

TEST_SRC = tests/test_keysymbols.cpp \
           tests/test_bindmodel.cpp \
           tests/test_grouping.cpp \
           tests/test_layout.cpp \
           tests/test_commands.cpp

CXXFLAGS += -std=c++26 -Wall -O2

# Default: build against installed Hyprland headers (hyprland.pc).
# To build against a source checkout instead:
#   make HYPRLAND_HEADERS=/path/to/Hyprland
ifdef HYPRLAND_HEADERS
INCLUDES = -I$(HYPRLAND_HEADERS) -I$(HYPRLAND_HEADERS)/src -I$(HYPRLAND_HEADERS)/protocols \
	`pkg-config --cflags hyprlang hyprutils hyprcursor hyprgraphics aquamarine pixman-1 libdrm pangocairo libinput libudev wayland-server xkbcommon`
else
INCLUDES = `pkg-config --cflags hyprland pixman-1 libdrm pangocairo libinput libudev wayland-server xkbcommon`
endif

PLUGIN_LIBS = `pkg-config --libs pangocairo`

all: $(TARGET)

$(TARGET): $(PLUGIN_SRC)
	$(CXX) $(CXXFLAGS) -shared -fPIC --no-gnu-unique -Isrc $(INCLUDES) $(PLUGIN_SRC) -o $@ $(PLUGIN_LIBS)

# Unit tests: domain + commands only, plain host build (no Hyprland headers).
TEST_BIN = build/hyprshortcuts_tests
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(DOMAIN_SRC) $(TEST_SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc $(DOMAIN_SRC) $(TEST_SRC) `pkg-config --cflags --libs gtest gtest_main` -o $@

clean:
	rm -f $(TARGET)
	rm -rf build

.PHONY: all test clean
