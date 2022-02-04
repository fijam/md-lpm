# md-lpm

This project consists of hardware files and software for a coin-battery operated laser tester that fits in a MiniDisk case.

## Hardware

Schematics, BOM and PCB layout files are available in the hardware directory.

## Theory of operation

This is the nerdy bit, feel free to skip ahead.

The heart of this device is a silicon PIN photodiode (BPW34) operated in reverse-biased mode, which generates current proportional to light intensity. The MiniDisc standard uses 780nm near-infrared laser source for both playback (using the magneto-optic Kerr effect) and recording (for heating up the disc to its Curie temperature). The output power spans between 0.5 and 9mW, depending on the device. 

The selected photodiode has approx. 95% spectral sensitivity at 780nm. The reverse voltage of 3V from a manganese-dioxide lithium battery and load resistance of 1k Ohm allows for a linear response up to 5.5mW as given by: 
`Psat = (0.3 + VR)/RL * S` where VR is the reverse voltage [2.9 V], S is the photosensitivity at 780nm [0.58 A/W] and RL is the load resistance [1000 Ohm]. Photodiode series resistance is disregarded.

High correlation between the photodiode readout (10b referenced to Vdd) and the device refPw setting (in decimal) is achieved:

![image](https://user-images.githubusercontent.com/75824/152569504-2ecc7f6e-f494-41f2-9b11-d20c15aa41ff.png)

Shorter fragments of this polynomial (around 0.5-1mW for LrefPw/HrefPw and 4.5-5.5mW for WritPW) can be approximated with a linear regression. 

To discount the effect of battery voltage dropping as it discharges, internal voltage reference of 1.1V is used for LrefPw/HrefPw. For WritPw, the whole range of Vdd is used. The setting of WritPw does not require a high precision.

Each device comes with a calibration factor saved in `user_row` area of the EEPROM which is protected from accidental chip erase. This calibration factor (by default 100%) accounts for differences between components, photodiode alignment and assembly in the minidisc case. It is taken into account when calculating LrefPw and HrefPw.

The firmware of the microcontroller is configured with a BOD of ~2.6V which prevents the device from running on a discharged battery.
