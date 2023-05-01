package main

import (
	"bufio"
	"fmt"
	"os"
	"regexp"
	"strings"
)

var dispAbs8Regexp = regexp.MustCompile(`^.*([+-]\$[a-fA-F0-9]{2}).*#(\$[a-fA-F0-9]{2})$`)
var abs16Regexp = regexp.MustCompile(`^.*(\$[a-fA-F0-9]{4}).*$`)

// readLines reads a whole file into memory
// and returns a slice of its lines.
func readLines(path string) ([]string, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var lines []string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}
	return lines, scanner.Err()
}

func isHex(v byte) bool {
	return ('A' <= v && v <= 'F') || ('0' <= v && v <= '9')
}

func determineParamMode(bytes []string, instr string) (paramMode, instrOut string) {
	if instr == "" {
		return "NONE", "???"
	}
	switch {
	case bytes[0] == "CB":
		// No params, not need to change instr
		return "NONE", instr
	case bytes[0] == "DD":
		if len(bytes) > 2 && bytes[1] == "CB" {
			idx := strings.Index(instr, "$"+bytes[2])
			return "DISP", instr[0:idx-1] + "%s" + instr[idx+3:]
		} else {
			switch len(bytes) {
			case 2:
				// No params, not need to change instr
				return "NONE", instr
			case 3:
				// REL, ABS8, or DISP
				idx := strings.Index(instr, "$"+bytes[2])
				if idx >= 0 {
					return "ABS8", instr[0:idx] + "%s" + instr[idx+3:]
				}
				idx = strings.Index(instr, "$")
				if idx < len(instr)-4 && isHex(instr[idx+1]) && isHex(instr[idx+2]) && isHex(instr[idx+3]) && isHex(instr[idx+4]) {
					return "REL", instr[0:idx] + "%s" + instr[idx+5:]
				}
				return "DISP", instr[0:idx-1] + "%s" + instr[idx+3:]
			case 4:
				// ABS16 or DISP_ABS8
				matches := abs16Regexp.FindStringSubmatchIndex(instr)
				if matches != nil {
					return "ABS16", instr[0:matches[2]] + "%s" + instr[matches[3]:]
				}
				matches = dispAbs8Regexp.FindStringSubmatchIndex(instr)
				return "DISP_ABS8", instr[0:matches[2]] + "%s" + instr[matches[3]:matches[4]] + "%s" + instr[matches[5]:]
			}
			return "NONE", "???"
		}
	case bytes[0] == "ED":
		switch len(bytes) {
		case 2:
			// No params, not need to change instr
			return "NONE", instr
		case 4:
			idx := strings.Index(instr, "$")
			return "ABS16", instr[0:idx] + "%s" + instr[idx+5:]
		}
		return "NONE", "???"
	case bytes[0] == "FD":
		if len(bytes) > 2 && bytes[1] == "CB" {
			idx := strings.Index(instr, "$"+bytes[2])
			return "DISP", instr[0:idx-1] + "%s" + instr[idx+3:]
		} else {
			switch len(bytes) {
			case 1:
				// No params, not need to change instr
				return "NONE", instr
			case 3:
				// REL, ABS8, or DISP
				idx := strings.Index(instr, "$"+bytes[1])
				if idx >= 0 {
					return "ABS8", instr[0:idx] + "%s" + instr[idx+3:]
				} else {
					idx = strings.Index(instr, "$")
					if idx < len(instr)-4 && isHex(instr[idx+1]) && isHex(instr[idx+2]) && isHex(instr[idx+3]) && isHex(instr[idx+4]) {
						return "REL", instr[0:idx] + "%s" + instr[idx+5:]
					} else {
						return "ABS8", instr[0:idx-1] + "%s" + instr[idx+3:]
					}
				}
			case 4:
				idx := strings.Index(instr, "$")
				return "ABS16", instr[0:idx] + "%s" + instr[idx+5:]
			}
			return "NONE", "???"
		}
	default:
		switch len(bytes) {
		case 1:
			// No params, not need to change instr
			return "NONE", instr
		case 2:
			// REL or ABS8
			idx := strings.Index(instr, "#$")
			if idx >= 0 {
				return "ABS8", instr[0:idx] + "#%s" + instr[idx+4:]
			} else {
				idx := strings.Index(instr, "$")
				// Count hex chars after $
				cnt := 0
				for idx+1+cnt < len(instr) && isHex(instr[idx+1+cnt]) {
					cnt++
				}
				if cnt == 2 {
					return "ABS8", instr[0:idx] + "%s" + instr[idx+1+cnt:]
				} else {
					return "REL", instr[0:idx] + "%s" + instr[idx+1+cnt:]
				}
			}
		case 3:
			idx := strings.Index(instr, "$")
			return "ABS16", instr[0:idx] + "%s" + instr[idx+5:]
		}
		panic("Can't happen!")
	}
}

type mode int

func main() {
	lines, _ := readLines("z80-opcodes")

	idx := 0

	pos := 0
	for idx < len(lines) {

		// skip until section start
		if !strings.HasPrefix(lines[idx], "----") {
			idx++
			continue
		}

		var tableLine []string
		var initMem []string
		var testLine []string

		// Analyze prefix
		prefixLine := lines[idx]
		idx++
		varName := "opcodes"
		if prefixLine == "---- prefix cb" {
			varName = "opcodes_cb"
		} else if prefixLine == "---- prefix dd" {
			varName = "opcodes_dd"
		} else if prefixLine == "---- prefix ed" {
			varName = "opcodes_ed"
		} else if prefixLine == "---- prefix fd" {
			varName = "opcodes_fd"
		} else if prefixLine == "---- prefix dd cb" {
			varName = "opcodes_ddcb"
		} else if prefixLine == "---- prefix fd cb" {
			varName = "opcodes_fdcb"
		}
		tableLine = append(tableLine, fmt.Sprintf("InstrDesc %s[256] = {", varName))

		// process section

		pos = 0
		for idx < len(lines) && strings.HasPrefix(lines[idx], ".C:") {
			l := lines[idx]
			idx++
			addr := l[3:7]
			bytes := strings.Split(strings.TrimSpace(l[9:20]), " ")
			rawInstr := strings.TrimSpace(l[21:])
			instr := rawInstr
			illegal := instr[0] == '*'
			if illegal {
				instr = instr[1:]
			}

			// Figure out address mode
			paramMode, instr := determineParamMode(bytes, instr)
			tableLine = append(tableLine, fmt.Sprintf("  /* 0x%02x */  {\"%s\", %s, %t},", pos, instr, paramMode, illegal))

			// fill initMem lines
			l = ""
			for _, b := range bytes {
				l = l + "0x" + b + ","
			}
			initMem = append(initMem, l)

			// fill test lines
			if rawInstr == "*" {
				rawInstr = "???"
			}
			byteStr := l[0 : len(l)-1]
			testLine = append(testLine, fmt.Sprintf("initMem(0x%s, { %s });", addr, byteStr))
			testLine = append(testLine, fmt.Sprintf("lines = disassembler_.disassembleForward(0x%s, memory_, 1);", addr))
			testLine = append(testLine, fmt.Sprintf("verifyLine(lines[0], 0x%s, { %s }, \"%s\");", addr, byteStr, rawInstr))

			pos++

		}

		tableLine = append(tableLine, fmt.Sprintf("};"))

		for _, s := range tableLine {
			fmt.Println(s)
		}
		fmt.Printf("==============================================================\n")
		// for _, s := range initMem {
		// 	fmt.Println(s)
		// }
		// fmt.Printf("--------------------------------------------------------------\n")
		for _, s := range testLine {
			fmt.Println(s)
		}
		fmt.Printf("==============================================================\n")

	}

}
