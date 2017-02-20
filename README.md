# dms-storage
DMS Project Storage Management System

# Setup
download latest raspian lite, sudo apt-get update, upgrade. 
set keyboard locale
set hostname

sudo apt-get install python-pip python-dev build-essential
sudo apt-get install libjpeg-dev
pip install python-imaging
sudo pip install pyserial
sudo pip install pillow
sudo pip install python-escpos
https://python-escpos.readthedocs.io/en/latest/user/installation.html


Run this python to test out printer:
```python
from escpos.printer import Usb
p = Usb(0x04b8, 0x0e15, 0)
p.text("Hello World\n")
p.barcode('1324354657687', 'EAN13', 64, 2, '', '')
p.cut()
```

Download latest go, https://golang.org/doc/install
Add go to PATH
sudo apt-get install git
go get github.com/brandonagr/dms-storage

go build dms-storage.go

Make it auto start:
sudo nano /etc/rc.local
 
iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to 8080
cd /home/pi/go/src/github.com/brandonagr/dms-storage
./dms-storage >> /var/log/storage.log 2>&1 &
