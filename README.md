Modified HexBright Code 
=======================
Features from the various demo examples and some additional functionality from
users jaebird and bhimoff have been combined into a basic low-med-high-dazzle
firmware and an accelerometer-driven adjustable brightness firmware.  I find
these two cases to be the most useful. The sketches have been placed in folders
for Arduino IDE compatibility. I plan to also add additional power-saving 
features (sleep modes) beyond the timed power-off to help extend battery life. 
I realize the sleep mode savings will be dwarfed by the LED current drain, but 
every mAh counts.

HexBright_Basic
-----------------
Based on the factory firmware with some modifications: button presses cycle 
through off, low, medium, and high modes; hold down the button while off for 
dazzle mode; hold down for one second in low or medium mode to turn off; auto
power-off based on accelerometer non-motion.

HexBright_Adjust
-----------------
Based on the hexbright4 firmware to adjust the brightness based on tilt
orientation. I plan to have the set brightness stored (EEPROM) and recalled at
next power-on.

HexBright Demo Code 
=======================
The following have been unmodified code-wise.  I have re-named them and put
them in folders like the modified ones.

HexBright_Demo_Morse
--------------------
Flashes out a message in morse code every time you press the button.  Nothing 
else.  The message and speed are easy to change- you can see and change both 
in the first lines of code.

HexBright_Demo_Taps
-------------------
Hold the button down, and with your other hand firmly tap on the light.  Tap
some more times, and let go of the button.  The exact sequence of taps will
be recorded and then played back as flashes until you press the button again
to turn off.

HexBright_Demo_Momentary
------------------------  
Light turns on only while the button is being held down.  That's it.

Removed Demo Code 
=======================
I removed the dazzle and fade code as separate sketches since they didn't seem
all that useful by themselves when the functionality can be added to the Basic
and/or Adjust code.
