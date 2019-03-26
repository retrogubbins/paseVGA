# pase
An emulation of the ZXSpectrum computer on an ESP32 and VGA Screen

VGA Driver is from ESP32Lib by BitLuni

https://github.com/bitluni/ESP32Lib

Z80 core is by Marcel de Kogel

Will load Manic Miner SNA snapshot from SDCard

Comment out the 'load_speccy' line in Setup to boot normally

Bugs: 

For some reason the SNA snapshot will not run properly using the RETN from the pushed PC on the stack, 
the code in 'tapes' has a hardcoded jump to the manic miner startup.

At present none of the other SNAs I have tried will run properly, I think this is possibly an interrupt
related problem.
