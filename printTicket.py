#!/usr/bin/python

from escpos.printer import Usb
from sys import argv

#expirationDate = "December 25"
#name = "Brandon Green"
#email = "brandonagr@gmail.com"
#type = "pallet"
#description="testing out storage"
#printTime = "2017-02-20 21:52"

def printTicketFunc(expirationDate, name, email, type, description, printTime):
	p = Usb(0x04b8, 0x0e15, 0)

	p.set(align='center', text_type='B', width=4, height=4)
	p.text(expirationDate)
	p.text("\n")

	p.set(align='center', width=1, height=1)
	p.text("storage expiration date\n\n")

	p.set(align='center', width=2, height=2)
	p.text("DMS Storage Ticket\n\n")
	p.set(align='left')
	p.text("Ticket required on any items left at DMS.\n")
	p.text("Place ticket in holder or on project.\n\n")

	p.set(align='left')
	p.text(str.format("Name:\t{}\n", name))
	p.text(str.format("Email:\t{}\n", email))
	p.text(str.format("Type:\t{}\n", type))
	p.text(str.format("Desc:\t{}\n", description))
	p.text(str.format("Start:\t{}\n", printTime))

	p.text("\n\n")
	p.text("Signature: ____________________________________\n")
	p.text("By signing you agree to follow the posted rules and remove your item before the expiration date.\n")
	p.text("Failure to remove items will result in loss of\nstorage privileges.")

	p.set(align='center')
	p.text("\n")
	qrDataString = str.format("{};{};{};{}", name, email, type, printTime)
	p.qr(qrDataString, 1, 6, 2)

	p.cut()

	p.set(align='center', width=2, height=2)
        p.text("DMS Storage Receipt\n\n")

        p.set(align='left', width=1, height=1)
        p.text("Keep this receipt as a reminder that you\n")
        p.text("agreed to remove your item before:\n")
	p.set(align='center', text_type='B', width=2, height=2)
	p.text(expirationDate)
	p.text("\n\n")

        p.set(align='left')
        p.text(str.format("Name:\t{}\n", name))
        p.text(str.format("Email:\t{}\n", email))
        p.text(str.format("Type:\t{}\n", type))
        p.text(str.format("Desc:\t{}\n", description))
        p.text(str.format("Start:\t{}\n", printTime))
	p.cut()



printTicketFunc(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6])
