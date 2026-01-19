# AudioTracker CLAP Plugin Makefile

CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -fPIC
LDFLAGS = -shared -framework Accelerate -lcurl

# Include paths
INCLUDES = -I./clap/include

# Source files
SRCS = src/plugin.cpp

# Output
PLUGIN_NAME = AudioTracker
BUNDLE_NAME = $(PLUGIN_NAME).clap
INSTALL_DIR = /Library/Audio/Plug-Ins/CLAP

# Build targets
.PHONY: all clean install debug bundle

all: bundle

# Compile the binary
$(PLUGIN_NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

# Create macOS bundle structure
bundle: $(PLUGIN_NAME)
	rm -rf $(BUNDLE_NAME)
	mkdir -p $(BUNDLE_NAME)/Contents/MacOS
	cp $(PLUGIN_NAME) $(BUNDLE_NAME)/Contents/MacOS/
	echo '<?xml version="1.0" encoding="UTF-8"?>' > $(BUNDLE_NAME)/Contents/Info.plist
	echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '<plist version="1.0">' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '<dict>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <key>CFBundleExecutable</key>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <string>$(PLUGIN_NAME)</string>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <key>CFBundleIdentifier</key>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <string>com.audiotracker.clap</string>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <key>CFBundleName</key>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <string>$(PLUGIN_NAME)</string>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <key>CFBundlePackageType</key>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <string>BNDL</string>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <key>CFBundleVersion</key>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '    <string>1.0.0</string>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '</dict>' >> $(BUNDLE_NAME)/Contents/Info.plist
	echo '</plist>' >> $(BUNDLE_NAME)/Contents/Info.plist

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: bundle

install: bundle
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE_NAME)
	cp -R $(BUNDLE_NAME) $(INSTALL_DIR)/

clean:
	rm -rf $(PLUGIN_NAME) $(BUNDLE_NAME)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE_NAME)
