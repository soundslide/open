# Firmware for SoundSlide Device
This repository contains the firmware for the SoundSlide device, powered by the SAMD11D14A microcontroller.

## Prerequisites

Install the necessary dependencies. On Ubuntu, run:

```sh
sudo apt install git make gcc-arm-none-eabi openocd npm
sudo npm install -g burgrp/silicon
```

## Building the Firmware

To build the firmware, first clone the repository and resolve firmware dependencies.

```sh
git clone https://github.com/soundslide/open.git soundslide
cd soundslide/fw
make deps
```

The `make deps` command installs dependencies and builds the initial firmware image. The ELF file will be located at `build/build.elf`.

For subsequent builds, simply run:
```sh
make build
```

## Flashing

You have two options for flashing the firmware:

### Option 1: Using SWD

For unprogrammed chips, use SWD via a JTAG connection to the C (clock) and D (data) pads on the PCB.

In one terminal, start `openocd` (assuming a CMSIS-DAP adapter is connected):

```sh
openocd -f interface/cmsis-dap.cfg -f target/at91samdXX.cfg
```

In a separate terminal, run:
```sh
make flash
```

### Option 2: Using USB

For subsequent updates, use the [SoundSlide CLI](../cli) utility:

```sh
ssc upgrade build/build.elf
```
