package main

import (
	"fmt"
	"os"
	"soundslide/cli/pkg/soundslide"
)

func main() {
	app := soundslide.NewApp()
	if err := app.Run(os.Args); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
