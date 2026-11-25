AquaOS

A tiny shell-based operating system for Arduino UNO R3 (and other small boards).
One sketch, 50 commands, LED/GPIO control, an EEPROM file system, and a trainable
50-parameter neural network.

Features

• Linux-style shell over the serial port (help, uname, whoami, ls, cat, rm, pwd, df, echo, clear, reboot...)
• LED and GPIO control (on, off, toggle, blink, pwm, fade, sos, strobe, ...)
• EEPROM file system with up to 32 small files
• Tiny AI: a 3-8-2 neural network with exactly 50 parameters that you can train on the board
• Small footprint: fixed small buffers, no dynamic allocation
• Portable: AVR-specific parts (reboot, peek/poke, free RAM) are guarded so the sketch can be adapted to other boards

Getting started

1. Install the Arduino IDE.
2. Put AquaOS.ino in a folder named AquaOS, then open it.
3. Select Tools > Board > Arduino UNO and the right port.
4. Click Upload.
5. Open Serial Monitor, set 9600 baud and Newline.
6. Type help.
AquaOS 1.0 - 'help' yaz
aqua> on 13
aqua> blink 13 5 200
aqua> add 2 3
5

Pins can be written as numbers (13) or analog names (a0).

Commands (50)

Group
Commands
System
help ver uname whoami hostname uptime free clear reboot echo pwd df
LED / GPIO
on off toggle blink pwm fade sos strobe pinmode dw dr ar pulse sleep rand
Math
add sub mul div mod hex bin seq
Memory
peek poke eer eew
Files
ls cat save rm
AI
ai-add ai-train ai-run ai-show ai-reset ai-save ai-load

LED / GPIO

Command
Description
on [pin] / off [pin]
Turn a pin on or off (default 13)
toggle [pin]
Flip a pin
blink [pin] [times] [ms]
Blink a pin (default 13, 5 times, 200 ms)
pwm [pin] [0-255]
PWM output (default pin 9)
fade [pin]
Fade up and down (default pin 9)
sos [pin]
Morse SOS
strobe [pin] [times]
Fast flashing
pinmode pin o|i|p
Output, input, or input with pull-up
dw pin 0|1 / dr pin
Digital write / read
ar pin
Analog read
pulse [pin] [ms]
One pulse
sleep ms
Wait
rand [max]
Random number

Math

add a b, sub a b, mul a b, div a b, mod a b, hex n, bin n, seq from to (max 50 numbers)

Memory and files

• peek addr / poke addr value: read/write a byte at a memory address (AVR). Be careful, a wrong address can crash the board.
• eer addr / eew addr value: read/write a byte in EEPROM.
• save name text: save a file (name up to 5 characters, text up to 9 characters).
• ls, cat name, rm name: list, show, and delete files.
• df: EEPROM size and file usage.

Files live in EEPROM starting at address 64. Weights saved by ai-save use addresses 0 to 50.

The AI

The network has 3 inputs, 8 hidden neurons (ReLU) and 2 outputs. That makes
24 + 8 + 16 + 2 = 50 parameters, stored as 8-bit integers. Training is a
simple random hill-climbing search that keeps a change only if the error does
not get worse.

Command
Description
ai-add a b c t1 t2
Add a training example: 3 inputs, 2 targets (values -100 to 100, max 8 examples)
ai-train [n]
Train for n steps (default 500, max 5000), prints the remaining error
ai-run a b c
Run the network
ai-show
Print all 50 weights
ai-reset
Randomize weights and clear examples
ai-save / ai-load
Save / load weights in EEPROM

Example:

aqua> ai-add 50 0 0 80 -20
aqua> ai-add 0 50 0 -20 80
aqua> ai-train 1000
aqua> ai-run 50 0 0

This is a toy model for learning and experiments, not a production ML system.

Notes

• Check the compiled size in the Arduino IDE output to see flash and RAM usage on your board.
• The Arduino UNO R3 has 2 KB of RAM and 32 KB of flash. Buffers are kept small so the sketch can also fit on smaller boards.
• reboot jumps to address 0 on AVR and calls ESP.restart() on ESP boards. It is not supported elsewhere.

License

MIT. Add a LICENSE file to your repository.
