#!/usr/bin/env bash

echo "Installing drivers for interating with maquette trains"

echo "installing libusb-1.0-0-dev"

sudo apt -y install libusb-1.0-0-dev

echo "Done"

echo "installing libredsusb-1.2.0"

cd libredsusb-1.2.0/
make
sudo make install

cd ..
echo "installing libmarklin-0.2.0"

cd libmarklin-0.2.0/
make
sudo make install
