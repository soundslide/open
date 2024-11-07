# Command Line tools for SoundSlide Device

This repository contains the command line tools for  for the SoundSlide device.

## Prerequisites

Install the necessary dependencies. On Ubuntu, run:

```sh
sudo apt install git make golang libusb-1.0-0-dev
```

## Building

Clone the repository and make it:

```sh
git clone https://github.com/soundslide/open.git soundslide
cd soundslide/cli
make
```

And then:

```sh
./ssc

NAME:
   ssc - SoundSlide configuration tool

USAGE:
   ssc [global options] command [command options]

COMMANDS:
   list      Lists connected SoundSlide devices
   set       Sets a parameter on the device
   get       Gets a parameter from the device
   defaults  Resets all parameters to default values
   upgrade   Upgrades the firmware
   help, h   Shows a list of commands or help for one command

GLOBAL OPTIONS:
   --serial value  Filter devices by serial number
   --help, -h      show help

```