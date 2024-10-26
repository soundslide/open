package soundslide

import (
	"debug/elf"
	"fmt"
	"os"
)

func elfToBinary(elfFileName string) ([]byte, error) {

	f, err := os.Open(elfFileName)
	if err != nil {
		return nil, fmt.Errorf("error opening file: %v", err)
	}
	defer f.Close()

	e, err := elf.NewFile(f)
	if err != nil {
		return nil, fmt.Errorf("error parsing ELF file: %v", err)
	}

	totalLen := 0
	for _, prog := range e.Progs {
		if prog.Type == elf.PT_LOAD && prog.Filesz > 0 {
			totalLen += int(prog.Memsz)
		}
	}

	data := make([]byte, totalLen)
	for _, prog := range e.Progs {
		if prog.Type == elf.PT_LOAD && prog.Filesz > 0 {
			prog.ReadAt(data[prog.Paddr:prog.Paddr+prog.Memsz], 0)
		}
	}
	return data, nil
}
