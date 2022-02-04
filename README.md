# md-lpm

This project consists of hardware files and software for a coin-battery operated laser tester that fits in a MiniDisk case.

## Theory of operation

This is the nerdy bit, feel free to skip ahead.

The heart of this device is a silicon PIN photodiode (BPW34) operated in reverse-biased mode, which generates current proportional to light intensity. The MiniDisc standard uses 780nm near-infrared laser source for both playback (using the magneto-optic Kerr effect) and recording (for heating up the disc to its Curie temperature). The output power spans between 0.5 and 9mW, depending on the device. 

The selected photodiode has approx. 95% spectral sensitivity at 780nm. The reverse voltage of 3V from a manganese-dioxide lithium battery and load resistance of 1k Ohm allows for a linear response up to 5.5mW as given by: 
`Psat = (0.3 + VR)/RL * S` where VR is the reverse voltage [2.9 V], S is the photosensitivity at 780nm [0.58 A/W] and RL is the load resistance [1000 Ohm]. Photodiode series resistance is disregarded.

High correlation between the photodiode readout and the device refPw setting is achieved:

