// A webserver that serves up the StorageTicket request page and handles talking to an attached receipt printer

package main

import (
	"fmt"
	"html"
	"io/ioutil"
	"net/http"
)

func ticketAPIHandler(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()
	body, _ := ioutil.ReadAll(r.Body)

	fmt.Fprintf(w, "Hello, %q %s", html.EscapeString(r.URL.Path), body)
}

func main() {
	http.Handle("/", http.FileServer(http.Dir("./wwwroot")))
	http.HandleFunc("/ticketApi/", ticketAPIHandler)

	fmt.Println("Hosting...")
	http.ListenAndServe(":8080", nil)
}
