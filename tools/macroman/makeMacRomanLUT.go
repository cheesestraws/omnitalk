package main

import (
	"fmt"
	"log"
	"os"
)

const unrecognised string = "\xEF\xBF\xBD"

func escapedString(bs []byte) string {
	var s string
	for _, b := range bs {
		s += fmt.Sprintf("\\x%02x", b);
	}
	return s
}

func main() {
	bs, err := os.ReadFile("macroman.txt")
	if err != nil {
		log.Fatal(err)
	}

	var strs [][]byte
	var accum []byte

	for _, b := range bs {
		if b != 9 && b != 10 && b != 13{
			accum = append(accum, b)
		} else {
			if len(accum) > 0 {
				strs = append(strs, accum)
				accum = nil
			}
		}
	}

	if len(accum) > 0 {
		strs = append(strs, accum)
	}
	
	if len(strs) != 128 {
		log.Fatal("wrong number of chars: ", len(strs));
	}

	
	var charmap []string
	var charlengths []int

	// Control characters are all "unrecognised" because I'm lazy
	unrecognisedEscape := escapedString([]byte(unrecognised))
	for i := 0; i < 32; i++ {
		charmap = append(charmap, unrecognisedEscape)
		charlengths = append(charlengths, len(unrecognised))
	}
	
	for i := 32; i < 127; i++ {
		charmap = append(charmap, escapedString([]byte{byte(i)}))
		charlengths = append(charlengths, 1)
	}
	
	// Del is unrecognised
	charmap = append(charmap, unrecognisedEscape)
	charlengths = append(charlengths, len(unrecognised))
	
	// Then the remaining 128 specials
	for _, bs := range strs {
		charmap = append(charmap, escapedString(bs))
		charlengths = append(charlengths, len(bs))
	}	
	
	if len(charmap) != 256 {
		log.Fatal("wrong number of chars in map");
	}
	
	if len(charlengths) != 256 {
		log.Fatal("wrong number of chars in charlengths");
	}
	
	fmt.Printf("#include <stddef.h>\n\n")
	
	// print the bugger out
	fmt.Printf("char* macroman_to_utf8[] = {\n\t")
	linelen := 0
	for i := 0; i < 256; i++ {
		linelen += len(charmap[i]) + 4
		fmt.Printf("\"%s\", ", charmap[i])
		if linelen >= 55 {
			linelen = 0
			fmt.Printf("\n\t")
		}
	}	
	
	fmt.Printf("};\n\n")
	
	fmt.Printf("size_t macroman_to_utf8_lengths[] = {\n\t")
	for i := 0; i < 256; i++ {
		fmt.Printf("%d, ", charlengths[i]);
		if i != 0 && i%20 == 0 {
			fmt.Printf("\n\t")
		}
	}
	fmt.Printf("};\n\n")
}
