.PHONY: build test upload monitor
build:
	pio run -e esp32-c3-supermini
test:
	pio test -e native
upload:
	pio run -e esp32-c3-supermini -t upload
monitor:
	pio device monitor -b 115200

