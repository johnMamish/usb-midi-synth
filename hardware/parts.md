 - samda1 / samdxx
   main microcontroller
   the samda1 appears to be pin-compatible with many different parts, and is reasonably affordable. Number of footprint-compatible variations makes me feel less worried about stock issues.

 - LM4808
   headphone amp
   very cheap headphone amplifier. Appears to be very widely stocked.

 - MCP73831/2
   LiPo charger
   affordable and widely stocked. Can implement charge sharing circuitry if we want the device to run while recharging.

 - CP2102N-xxx-xQFN20
   UART-to-USB bridge
   A laptop can use this interface to adjust synthesizer parameters

 - AP3012KTR
   Boost regulator for 5V USB
   Extremely common
   can use 30k / 10k feedback resistors

 - AP7366-W5 or LDK320
   LDO

 - some buttons of some sort

 - LEDs for low-battery indication

 - headphone jack

 - power button

 - SS-52100-001, AU-Y1005-2, or 0676433910
   USB host connector
   These parts are all footprint compatible.

 - USB4105-GF-A, USB4105-GF-A-060, USB4105-GF-A-120
   USB device connector

 - SWD connector

 - screen

 - capacitive touch sensors

Checklist
 - [ ] Button inputs to MCU
 - [x] Headphone connection
   - [x] DAC out from MCU goes to amp
   - [x] Headphone connector
 - [ ] microcontroller
   - [ ] reset button
   - [ ] decoupling
   - [ ] oscillator
