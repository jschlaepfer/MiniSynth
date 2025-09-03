# MiniSynth

My first VST synthesiser

This is a polyphonic synthesiser implemented in C++ (with JUCE).
Implemented and tested on MacOS (arm M1).

An external midi device (like a usb keyboard) is required to make sounds! 
If you are running the standalone version, any MIDI devices connected to your computer should be automatically detected.


![alt text](Resources/im/Minisynth_rev1.png)

Yes... I know... The graphical interface is ugly. ;-)

3 main oscillators, 1 noise generator, PWM, ...
Well, there's sound coming out. 
But the preset menu isn't working yet. 
The interface needs to be completely redesigned to make it more user-friendly. 
The ADSR, which should manage the amplitude envelope, doesn't seem to be working...