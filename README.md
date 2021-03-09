# TinyTX

This repo picks up on the work on Nathan Chantrell, sadly R.I.P. late 2020.

* Nathan's website [via Archive.org](https://web.archive.org/web/20150202220320/http://nathan.chantrell.net/tinytx-wireless-sensor/)
* Nathan's [TinyTX repo on Github](https://github.com/nathanchantrell/TinyTX)
* Archive of [TinyTX3 hardware](TinyTX3/)

## Motivation

Since Nathan created TinyTX, both chips have evolved:

1. RFM12B -> [RFM69](https://www.hoperf.com/modules/rf_transceiver/index.html)
2. ATtiny84 -> ATtiny 0,1 series e.g. [ATtiny1614](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny1614-16-17-DataSheet-DS40002204A.pdf)

## High level design

This project will create a TinyTX4 using:

1. RFM69
2. ATtiny1614
3. lowpowerlab's [RFM69](https://github.com/lowpowerlab/rfm69) library

And release all hardware and software in this repository.

## Radio module choice

Here is a comparison of [RFM69](https://www.hoperf.com/modules/rf_transceiver/index.html) variants:

Variant|Output (dBm)|Pinout|Size (mm)
---|---|---|---
RFM69[**W**](https://www.hoperf.com/modules/rf_transceiver/RFM69W.html)|13|RFM69W|19.7 x 16
RFM69[**CW**](https://www.hoperf.com/modules/rf_transceiver/RFM69C.html)|13|RFM69CW|16 x 16
RFM69[**HW**](https://www.hoperf.com/%20modules/rf_transceiver/RFM69HW.html)|20|RFM69W|19.7 x 16
RFM69[**HCW**](https://www.hoperf.com/modules/rf_transceiver/RFM69HCW.html)|20|RFM69HCW|16 x 16

i.e. RFM69W and RFM69HW share pinout, but are larger (19.7&nbsp;x&nbsp;16)mm vs the RFMCW and RFMHCW (16&nbsp;x&nbsp;16)mm.

For interoperability between normal and high-power modules, we'll go with the **RFM69(H)W** modules.

## Hardware versions

* [TinyTX4](hardware/TinyTX4) (monopole and SMA)

![TinyTX4 Gif](https://user-images.githubusercontent.com/534681/110469152-3162a280-80d1-11eb-835b-dacc42af85ce.gif)
