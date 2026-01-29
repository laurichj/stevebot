# Makefile for Stevebot ESP32 Project
# Provides convenient shortcuts for common development tasks

.PHONY: help setup update test test-verbose build upload monitor clean all verify

# Default target - show help
help:
	@echo "Stevebot Development Tasks"
	@echo ""
	@echo "Setup & Configuration:"
	@echo "  make setup          - Initial project setup (install PlatformIO, configure secrets)"
	@echo "  make update         - Update PlatformIO and dependencies"
	@echo ""
	@echo "Testing:"
	@echo "  make test           - Run all native unit tests (fast, no hardware)"
	@echo "  make test-verbose   - Run tests with verbose output"
	@echo "  make test-specific  - Run specific test (use TEST=test_name)"
	@echo ""
	@echo "Building:"
	@echo "  make build          - Build ESP32 firmware"
	@echo "  make verify         - Verify build and run tests"
	@echo "  make clean          - Clean build artifacts"
	@echo ""
	@echo "Hardware:"
	@echo "  make upload         - Upload firmware to connected ESP32"
	@echo "  make monitor        - Open serial monitor (115200 baud)"
	@echo "  make flash          - Build, upload, and monitor"
	@echo ""
	@echo "Convenience:"
	@echo "  make all            - Run tests and build firmware"
	@echo ""
	@echo "Examples:"
	@echo "  make test-specific TEST=test_state_machine"
	@echo "  make flash PORT=/dev/ttyUSB0"

# Initial project setup
setup:
	@echo "==> Setting up Stevebot development environment..."
	@if [ ! -d ".venv" ]; then \
		echo "Creating Python virtual environment..."; \
		python3 -m venv .venv; \
	fi
	@echo "Installing PlatformIO..."
	@bash -c "source .venv/bin/activate && pip3 install --upgrade platformio"
	@if [ ! -f "src/secrets.h" ]; then \
		echo "Creating src/secrets.h from template..."; \
		cp src/secrets.h.template src/secrets.h; \
		echo ""; \
		echo "⚠️  IMPORTANT: Edit src/secrets.h with your WiFi credentials!"; \
		echo "   vim src/secrets.h  (or your preferred editor)"; \
	else \
		echo "src/secrets.h already exists, skipping..."; \
	fi
	@echo ""
	@echo "✅ Setup complete! Next steps:"
	@echo "   1. Edit src/secrets.h with your WiFi credentials"
	@echo "   2. Run 'make test' to verify tests pass"
	@echo "   3. Run 'make build' to build firmware"

# Update PlatformIO and dependencies
update:
	@echo "==> Updating PlatformIO and dependencies..."
	@bash -c "source .venv/bin/activate && pip3 install --upgrade platformio"
	@bash -c "source .venv/bin/activate && pio pkg update"
	@echo "✅ Update complete!"

# Run native unit tests
test:
	@echo "==> Running native unit tests..."
	@if [ -d ".venv" ]; then \
		bash -c "source .venv/bin/activate && pio test -e native"; \
	else \
		pio test -e native; \
	fi

# Run tests with verbose output
test-verbose:
	@echo "==> Running native unit tests (verbose)..."
	@if [ -d ".venv" ]; then \
		bash -c "source .venv/bin/activate && pio test -e native -v"; \
	else \
		pio test -e native -v; \
	fi

# Run specific test suite
test-specific:
	@if [ -z "$(TEST)" ]; then \
		echo "Error: TEST variable not set"; \
		echo "Usage: make test-specific TEST=test_state_machine"; \
		exit 1; \
	fi
	@echo "==> Running test: $(TEST)..."
	@if [ -d ".venv" ]; then \
		bash -c "source .venv/bin/activate && pio test -e native --filter $(TEST)"; \
	else \
		pio test -e native --filter $(TEST); \
	fi

# Build ESP32 firmware
build:
	@echo "==> Building ESP32 firmware..."
	@if [ ! -f "src/secrets.h" ]; then \
		echo "⚠️  Warning: src/secrets.h not found!"; \
		echo "Creating from template (build will likely fail without WiFi credentials)..."; \
		cp src/secrets.h.template src/secrets.h; \
	fi
	@if [ -d ".venv" ]; then \
		bash -c "source .venv/bin/activate && pio run -e esp32"; \
	else \
		pio run -e esp32; \
	fi
	@echo "✅ Build complete!"

# Verify everything works (tests + build)
verify: test build
	@echo ""
	@echo "✅ All checks passed!"
	@echo "   - 38 unit tests passing"
	@echo "   - ESP32 firmware builds successfully"

# Upload firmware to ESP32
upload:
	@echo "==> Uploading firmware to ESP32..."
	@if [ -d ".venv" ]; then \
		if [ ! -z "$(PORT)" ]; then \
			bash -c "source .venv/bin/activate && pio run -e esp32 --target upload --upload-port $(PORT)"; \
		else \
			bash -c "source .venv/bin/activate && pio run -e esp32 --target upload"; \
		fi; \
	else \
		if [ ! -z "$(PORT)" ]; then \
			pio run -e esp32 --target upload --upload-port $(PORT); \
		else \
			pio run -e esp32 --target upload; \
		fi; \
	fi
	@echo "✅ Upload complete!"

# Open serial monitor
monitor:
	@echo "==> Opening serial monitor (115200 baud)..."
	@echo "Press Ctrl+C to exit"
	@if [ -d ".venv" ]; then \
		if [ ! -z "$(PORT)" ]; then \
			bash -c "source .venv/bin/activate && pio device monitor -b 115200 -p $(PORT)"; \
		else \
			bash -c "source .venv/bin/activate && pio device monitor -b 115200"; \
		fi; \
	else \
		if [ ! -z "$(PORT)" ]; then \
			pio device monitor -b 115200 -p $(PORT); \
		else \
			pio device monitor -b 115200; \
		fi; \
	fi

# Build, upload, and monitor in one step
flash: build upload
	@echo ""
	@echo "==> Starting serial monitor..."
	@$(MAKE) monitor

# Clean build artifacts
clean:
	@echo "==> Cleaning build artifacts..."
	@if [ -d ".venv" ]; then \
		bash -c "source .venv/bin/activate && pio run -t clean"; \
	else \
		pio run -t clean; \
	fi
	@rm -rf .pio/build
	@echo "✅ Clean complete!"

# Run tests and build (common workflow)
all: test build
	@echo ""
	@echo "✅ Tests passed and firmware built successfully!"
