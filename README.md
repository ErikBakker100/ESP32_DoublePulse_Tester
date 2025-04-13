Method for Measuring Switching Parameters
The standard test method for measuring switching parameters of Si, SiC, and GaN MOSFETs and IGBTs is the double pulse Test (DPT). Double pulse testing can be used to measure energy loss during device turn-on and turn-off, as well as reverse recovery parameters.
A good explanation can be found here: https://www.tek.com/en/documents/application-note/double-pulse-test-tektronix-afg31000-arbitrary-function-generator?anv=2

Program runs on an ESP32. Minimum pulsewith = 1 micro second

Sending a Json like: {"pulseInterval": 500, "pulseWidth1": 10, "interPulseDelay": 200, "pulseWidth2": 10} via the serial port (115200 baud, n, 8, 1) generates
               _____________                   ____________
pulseInterval | pulseWidth1 | interPulseDelay | pulseWith2 | pulseInterval");
___ 500_______      10       _____ 200 ________      10      _____ 500 ____"

Software is partly used from Teensy 4.0 Signal Generator, Electronics Workshop, Robin O'Reilly. The code to generate pulses is placed in the second core of the ESP32, which generates more reliable pulses.
