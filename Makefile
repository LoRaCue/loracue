.PHONY: build flash clean

VID := 1209
PID := FABF
VID_DEC := 4617
PID_DEC := 64191

build:
	@mkdir -p build
	@cd build && cmake .. && make -j8
	@echo "Build complete: build/I2Console.uf2"

flash: build
	@echo "Looking for I2Console device (VID:PID = $(VID):$(PID))..."
	@if [ "$$(uname)" = "Darwin" ]; then \
		CDC_PORT=$$(ioreg -r -c IOUSBHostDevice -l | \
			awk '/metaneutrons/,/^  \|   \}/' | \
			grep "IOTTYSuffix" | tail -1 | sed 's/.*"\([0-9]*\)".*/\1/'); \
		if [ -n "$$CDC_PORT" ]; then \
			CDC_PATH="/dev/tty.usbmodem$$CDC_PORT"; \
			echo "Found I2Console at $$CDC_PATH"; \
			echo "Triggering bootloader..."; \
			(stty -f $$CDC_PATH 115200 && printf "BOOTLOADER\r\n" > $$CDC_PATH) 2>/dev/null & \
			sleep 3; \
		else \
			echo "I2Console not found."; \
			echo "Please enter bootloader mode manually:"; \
			echo "  1. Hold BOOTSEL button"; \
			echo "  2. Connect USB"; \
			echo "  3. Release BOOTSEL"; \
			echo ""; \
			echo "Press Enter when ready..."; \
			read dummy; \
		fi; \
	elif [ "$$(uname)" = "Linux" ]; then \
		CDC_PORT=$$(lsusb -d $(VID):$(PID) 2>/dev/null | head -1); \
		if [ -n "$$CDC_PORT" ]; then \
			TTY=$$(ls /dev/ttyACM* 2>/dev/null | tail -1); \
			if [ -n "$$TTY" ]; then \
				echo "Found I2Console at $$TTY"; \
				echo "Sending bootloader command..."; \
				printf "BOOTLOADER\n" > $$TTY; \
				sleep 2; \
			fi; \
		else \
			echo "I2Console not found."; \
			echo "Please enter bootloader mode manually:"; \
			echo "  1. Hold BOOTSEL button"; \
			echo "  2. Connect USB"; \
			echo "  3. Release BOOTSEL"; \
			echo ""; \
			echo "Press Enter when ready..."; \
			read dummy; \
		fi; \
	fi; \
	echo "Flashing firmware..."; \
	picotool load build/I2Console.uf2 && \
	picotool reboot && \
	echo "Flash complete!"

clean:
	@rm -rf build
	@echo "Build directory cleaned"
