// A webserver that serves up the StorageTicket request page and handles talking to an attached receipt printer

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"
	"os/exec"

	"github.com/gorilla/schema"
)

type ticketFormSubmission struct {
	Name        string `schema:"name"`
	Email       string `schema:"email"`
	Description string `schema:"description"`
	StorageType string `schema:"storageType"`
	SubmitTime  string `schema:"submitTime"`
	ExpireDate  string `schema:"expireTime"`
}

func (ticket *ticketFormSubmission) computeTime() {
	curTime := time.Now()
	expireTime := nthWeekdayOfMonth(time.Sunday, 1, curTime)
	if expireTime.Before(curTime) {
		expireTime = nthWeekdayOfMonth(time.Sunday, 1, curTime.AddDate(0, 1, 0))
	}

	ticket.SubmitTime = curTime.Format("2006-01-02 15:04")
	ticket.ExpireDate = expireTime.Format("January 2")
}

func nthWeekdayOfMonth(weekday time.Weekday, n int, date time.Time) time.Time {
	count := 0
	year, month, _ := date.Date()
	currentDate := time.Date(year, month, 1, 0, 0, 0, 0, time.UTC)
	for {
		if currentDate.Weekday() == weekday {
			count++
			if count == n {
				return currentDate
			}
		}
		currentDate = currentDate.AddDate(0, 0, 1)
	}
}

func ticketAPIHandler(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()

	err := r.ParseForm()
	if err != nil {
		http.Error(w, err.Error(), 400)
		return
	}

	ticket := new(ticketFormSubmission)

	decoder := schema.NewDecoder()
	err = decoder.Decode(ticket, r.PostForm)
	if err != nil {
		http.Error(w, err.Error(), 400)
		return
	}

	ticket.computeTime()


	cmd := exec.Command("python",  "printTicket.py", ticket.ExpireDate, ticket.Name, ticket.Email, ticket.StorageType, ticket.Description, ticket.SubmitTime)
        log.Print(cmd.Args)
        _, err = cmd.CombinedOutput()
        if err != nil { 
		log.Print(err.Error())
		http.Error(w, err.Error(), 500)
		return
 	}

	// just echo back body for now
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ticket)
}

func main() {
	http.Handle("/", http.FileServer(http.Dir("./wwwroot")))
	http.HandleFunc("/ticketApi/", ticketAPIHandler)

	fmt.Println("Hosting...")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
