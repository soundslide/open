package soundslide

import (
	"fmt"
	"strconv"
	"time"

	"github.com/urfave/cli/v2"
)

func getParameterKeys() string {
	keys := ""
	for key := range DeviceParameters {
		keys += key + "|"
	}
	return keys[:len(keys)-1]
}

func NewApp() *cli.App {
	app := &cli.App{
		Name:  "ssc",
		Usage: "SoundSlide configuration tool",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:  "serial",
				Usage: "Filter devices by serial number",
				//Destination: &serialNumberFilter,
			},
		},
		Commands: []*cli.Command{
			{
				Name:   "list",
				Usage:  "Lists connected SoundSlide devices",
				Action: listDevices,
			},
			{
				Name:      "set",
				Usage:     "Sets a parameter on the device",
				Action:    setParameter,
				Args:      true,
				ArgsUsage: "<" + getParameterKeys() + "> <value>",
			},
			{
				Name:      "get",
				Usage:     "Gets a parameter from the device",
				Action:    getParameter,
				Args:      true,
				ArgsUsage: "<" + getParameterKeys() + ">",
			},
			{
				Name:   "defaults",
				Usage:  "Resets all parameters to default values",
				Action: setDefaults,
			},
			{
				Name:      "upgrade",
				Usage:     "Upgrades the firmware",
				Action:    upgradeFirmware,
				Args:      true,
				ArgsUsage: "<elf-image>",
				Flags: []cli.Flag{
					&cli.BoolFlag{
						Name:  "progress",
						Usage: "Disable progress monitoring",
					},
				},
			},
		},
	}

	return app
}

func listDevices(c *cli.Context) error {

	devices, err := ListDevices(c.String("serial"))
	if err != nil {
		return err
	}

	if len(devices) == 0 {
		fmt.Println("No devices found")
		return nil
	}

	for _, d := range devices {
		fmt.Printf("%s version %d.%d.%d\n", d.SerialNumber, d.Version.Major, d.Version.Minor, d.Version.Patch)
	}
	return nil
}

func setParameter(c *cli.Context) error {

	device, err := OpenDevice(c.String("serial"))
	if err != nil {
		return fmt.Errorf("error opening device: %v", err)
	}

	key := c.Args().Get(0)
	if key == "" {
		return fmt.Errorf("key is required, valid keys are: %s", getParameterKeys())
	}

	valueStr := c.Args().Get(1)
	if valueStr == "" {
		return fmt.Errorf("value is required")
	}
	value, err := strconv.ParseUint(valueStr, 10, 8)
	if err != nil {
		return fmt.Errorf("value \"%v\" is not integer (%v)", valueStr, err)
	}

	err = device.SetParameter(key, uint8(value))
	if err != nil {
		return fmt.Errorf("error setting parameter: %v", err)
	}

	return nil
}

func getParameter(c *cli.Context) error {

	device, err := OpenDevice(c.String("serial"))
	if err != nil {
		return fmt.Errorf("error opening device: %v", err)
	}

	key := c.Args().Get(0)
	if key == "" {
		return fmt.Errorf("key is required, valid keys are: %s", getParameterKeys())
	}

	value, err := device.GetParameter(key)
	if err != nil {
		return fmt.Errorf("error getting parameter: %v", err)
	}

	fmt.Println(value)
	return nil
}

func setDefaults(c *cli.Context) error {

	device, err := OpenDevice(c.String("serial"))
	if err != nil {
		return fmt.Errorf("error opening device: %v", err)
	}

	err = device.SetDefaults()
	if err != nil {
		return fmt.Errorf("error setting defaults: %v", err)
	}

	return nil
}

func upgradeFirmware(c *cli.Context) error {

	imageFile := c.Args().Get(0)
	if imageFile == "" {
		return fmt.Errorf("elf-image is required")
	}

	device, err := OpenDevice(c.String("serial"))
	if err != nil {
		return fmt.Errorf("error opening device: %v", err)
	}
	defer device.Close()

	showProgress := !c.Bool("progress")

	err = device.UpgradeFirmware(imageFile, func(pagesWritten, totalPages int) {
		if showProgress {
			fmt.Printf("\r%d%% done", 100*pagesWritten/totalPages)
		}
	})
	if err != nil {
		return fmt.Errorf("error upgrading firmware: %v", err)
	}

	if err != nil {
		return fmt.Errorf("error closing device: %v", err)
	}

	for i := 0; ; i++ {
		device2, err := OpenDevice(c.String("serial"))
		if err != nil {
			if i == 10 {
				return fmt.Errorf("error opening device: %v", err)
			}
			time.Sleep(1 * time.Second)
			continue
		}
		defer device2.Close()
		fmt.Printf("%s version %d.%d.%d\n", device2.SerialNumber, device2.Version.Major, device2.Version.Minor, device2.Version.Patch)
		break
	}

	return nil
}
