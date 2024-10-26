package soundslide

import (
	"fmt"

	"github.com/google/gousb"
)

// keep in sync with usb-cfg.cpp
const (
	CFG_REQUEST_GET_STATUS = 0x01 // IN,  data: bytes [patch version high byte, patch version low byte]

	CFG_REQUEST_SET_PARAMETER = 0x10 // OUT, wValue low byte: parameter key, wValue high byte: parameter value
	CFG_REQUEST_GET_PARAMETER = 0x11 // IN,  wValue low byte: parameter key, data: parameter value (one byte)
	CFG_REQUEST_SET_DEFAULTS  = 0x12 // OUT, data: none

	CFG_REQUEST_IMG_PREPARE = 0x20 // OUT, wValue: image size in pages
	CFG_REQUEST_IMG_INSTALL = 0x21 // OUT, data: CRC32 of the image as big-endian uint32

	INTERFACE_STATE_IDLE       = 0
	INTERFACE_STATE_UPLOADING  = 1
	INTERFACE_STATE_INSTALLING = 2

	PAGE_SIZE = 64
)

var DeviceParameters map[string]uint8 = map[string]uint8{
	"flip":        0,
	"scale":       1,
	"sensitivity": 2,
}

type SoundSlideDevice struct {
	usbDevice   *gousb.Device
	fwuEndpoint *gousb.OutEndpoint

	SerialNumber string
	Version      struct {
		Major int
		Minor int
		Patch int
	}
}

type Status struct {
	PatchVersion int
}

func ListDevices(onlySerialNumber string) ([]SoundSlideDevice, error) {
	ctx := gousb.NewContext()
	defer ctx.Close()

	var ssDevices []SoundSlideDevice
	usbDevices, err := ctx.OpenDevices(func(desc *gousb.DeviceDesc) bool {
		return desc.Vendor == 0xF5A2 && desc.Product == 0x0001
	})
	if err != nil {
		return nil, fmt.Errorf("error opening device(s): %v", err)
	}

	for _, usbDevice := range usbDevices {
		serialNumber, err := usbDevice.SerialNumber()
		if err != nil {
			return nil, fmt.Errorf("error getting serial number: %v", err)
		}

		if onlySerialNumber == "" || onlySerialNumber == serialNumber {

			ssDevice := SoundSlideDevice{
				usbDevice:    usbDevice,
				SerialNumber: serialNumber,
				Version: struct {
					Major int
					Minor int
					Patch int
				}{
					Major: int(usbDevice.Desc.Device) >> 8,
					Minor: int(usbDevice.Desc.Device) & 0xff,
					Patch: 0,
				},
			}

			config, err := usbDevice.Config(1)
			if err != nil {
				return nil, fmt.Errorf("error getting config: %v", err)
			}

			cfgInterface, err := config.Interface(1, 0)
			if err != nil {
				return nil, fmt.Errorf("error getting interface: %v", err)
			}

			fwuEndpoint, err := cfgInterface.OutEndpoint(2)
			if err != nil {
				return nil, fmt.Errorf("error getting endpoint: %v", err)
			}
			ssDevice.fwuEndpoint = fwuEndpoint

			status, err := ssDevice.GetStatus()
			if err != nil {
				return nil, fmt.Errorf("error getting status: %v", err)
			}

			ssDevice.Version.Patch = status.PatchVersion

			ssDevices = append(ssDevices, ssDevice)

		}
	}

	return ssDevices, nil
}

func OpenDevice(onlySerialNumber string) (*SoundSlideDevice, error) {
	devices, err := ListDevices(onlySerialNumber)
	if err != nil {
		return nil, fmt.Errorf("error listing devices: %v", err)
	}

	if len(devices) == 0 {
		return nil, fmt.Errorf("no devices found")
	}

	if len(devices) > 1 {
		return nil, fmt.Errorf("multiple devices found, please specify a serial number")
	}

	return &devices[0], nil
}

func paramKeyToInt(key string) (uint8, error) {
	if value, ok := DeviceParameters[key]; ok {
		return value, nil
	}
	return 0, fmt.Errorf("unknown parameter: %s", key)
}

func (d SoundSlideDevice) configInterfaceRequestIn(bRequest uint8, wValue uint16, length int) ([]byte, error) {
	data := make([]byte, length)
	read, err := d.usbDevice.Control(
		gousb.ControlIn|gousb.ControlVendor|gousb.ControlInterface, // bmRequestType
		bRequest, // bRequest
		wValue,   // wValue
		0x0001,   // wIndex
		data)
	if err != nil {
		return nil, fmt.Errorf("error issuing control request: %v", err)
	}
	if read != len(data) {
		return nil, fmt.Errorf("expected %d bytes to be received, got %d", len(data), read)
	}

	return data, nil
}

func (d SoundSlideDevice) configInterfaceRequestOut(bRequest uint8, wValue uint16) error {
	_, err := d.usbDevice.Control(
		gousb.ControlOut|gousb.ControlVendor|gousb.ControlInterface, // bmRequestType
		bRequest, // bRequest
		wValue,   // wValue
		0x0001,   // wIndex
		nil)
	if err != nil {
		return fmt.Errorf("error issuing control request: %v", err)
	}

	return nil
}

func (d SoundSlideDevice) Close() {
	d.usbDevice.Close()
}

func (d SoundSlideDevice) GetStatus() (Status, error) {

	data, err := d.configInterfaceRequestIn(CFG_REQUEST_GET_STATUS, 0, 2)
	if err != nil {
		return Status{}, err
	}

	return Status{
		PatchVersion: int(data[0])<<8 | int(data[1]),
	}, nil
}

func (d SoundSlideDevice) SetParameter(key string, value uint8) error {
	keyIndex, err := paramKeyToInt(key)
	if err != nil {
		return err
	}

	err = d.configInterfaceRequestOut(CFG_REQUEST_SET_PARAMETER, uint16(value)<<8|uint16(keyIndex))
	if err != nil {
		return err
	}

	return nil
}

func (d SoundSlideDevice) GetParameter(key string) (uint8, error) {

	keyIndex, err := paramKeyToInt(key)
	if err != nil {
		return 0, err
	}

	data, err := d.configInterfaceRequestIn(CFG_REQUEST_GET_PARAMETER, uint16(keyIndex), 1)
	if err != nil {
		return 0, err
	}

	return data[0], nil
}

func (d SoundSlideDevice) SetDefaults() error {

	err := d.configInterfaceRequestOut(CFG_REQUEST_SET_DEFAULTS, 0)
	if err != nil {
		return err
	}

	return nil
}

func (d SoundSlideDevice) UpgradeFirmware(imageFile string, progressMonitor func(int, int)) error {

	data, err := elfToBinary(imageFile)
	if err != nil {
		return fmt.Errorf("error converting ELF to binary: %v", err)
	}
	pad := len(data) % PAGE_SIZE
	if pad > 0 {
		data = append(data, make([]byte, PAGE_SIZE-pad)...)
	}

	pages := len(data) / PAGE_SIZE
	progressMonitor(0, pages)

	err = d.configInterfaceRequestOut(CFG_REQUEST_IMG_PREPARE, uint16(pages))
	if err != nil {
		return fmt.Errorf("error preparing image: %v", err)
	}

	for i := 0; i < pages; i++ {
		written, err := d.fwuEndpoint.Write(data[i*PAGE_SIZE : (i+1)*PAGE_SIZE])
		if err != nil {
			return fmt.Errorf("error writing image: %v", err)
		}
		if written != PAGE_SIZE {
			return fmt.Errorf("short write")
		}
		progressMonitor(i+1, pages)
	}

	var crc16 uint16 = 0x1234
	for i := 0; i < len(data); i += 2 {
		crc16 ^= uint16(data[i]) | uint16(data[i+1])<<8
	}
	fmt.Print("\n")

	err = d.configInterfaceRequestOut(CFG_REQUEST_IMG_INSTALL, crc16)
	if err != nil {
		return fmt.Errorf("error installing image: %v", err)
	}

	return nil
}
