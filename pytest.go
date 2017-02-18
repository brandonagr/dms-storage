package main

import "fmt"
import "os/exec"

func main() {

	cmd := exec.Command("python",  "printTicket.py", "March 5", "Brandon Green", "brandonagr@gmail.com", "small", "short description", "2017-02-01 10:00")
	fmt.Println(cmd.Args)
	out, err := cmd.CombinedOutput()
	if err != nil { fmt.Println(err); }
	fmt.Println(string(out))
}
