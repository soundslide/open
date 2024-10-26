.PHONY: all flash build deps clean

BUILD=silicon build -s -o s --ldscript cortex-m0.ld

all: flash

flash:
	$(BUILD) -f

build:
	$(BUILD)

deps:
	$(BUILD) -d

publish: build
	arm-none-eabi-objcopy -O binary build/build.elf build/build.bin
	mv build/build.bin ../web/root/firmware/$(shell jq -r .version package.json).bin
	find ../web/root/firmware -name "*.bin" -printf '%P\n' > ../web/root/firmware/index.txt
	make -C ../web publish

clean:
	rm -rf build node_modules