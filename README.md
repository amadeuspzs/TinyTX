# TinyTX

This repo picks up on the work on Nathan Chantrell, sadly R.I.P. late 2020.

* Nathan's website [via Archive.org](https://web.archive.org/web/20150202220320/http://nathan.chantrell.net/tinytx-wireless-sensor/)
* Nathan's [TinyTX repo on Github](https://github.com/nathanchantrell/TinyTX)

## Motivation

Since Nathan created TinyTX, both chips have evolved:

1. RFM12B -> [RFM69](https://www.hoperf.com/modules/rf_transceiver/index.html)
2. ATtiny84 -> ATtiny 0,1 series e.g. [`ATtiny1614`](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny1614-16-17-DataSheet-DS40002204A.pdf)

And the radio module libraries have evolved.

## High level design

This project will create a TinyTX4 using:

1. RFM69
2. ATtiny1614
3. lowpowerlab's [RFM69](https://github.com/lowpowerlab/rfm69) library

And release all hardware and software in this repository.

Stay tuned for updates!
