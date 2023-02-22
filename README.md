# Internet Connected Pinball

A project to allow Williams System 7 pinball machines to automatically upload scores to a website.

[**Read more about the project here**](https://eli.lipsitz.net/posts/internet-connected-pinball/)

This repository has code and design files for the device that taps into the pinball machine's bus, and the web application that receives and displays the scores.

None of the code here is production quality, it's purely for reference and inspiration. 

## Contents
* `address-decoder`: Program for the ATF16V8 PLD chip, used as an address decoder
* `controller`: Raspberry Pi Pico W controller software
* `testing`: Software written to help test the device during development
* `pcb`: KiCad design files for the PCB
* `web`: Flask webapp showing pinball scores and score upload API
* `obsolete`: Obsolete software: original ESP32-based controller, RP2040 proof of concept
