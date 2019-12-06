# TFT Test Driver: #

TFT Test Driver is a mini project I used to help me define how to use the TFT touch screeen for controls and display


### What is this repository for? ###

* Provides a boiler plate method for headers
* Provides a repeatable for Screen displays
* Provides a seperation of Touch actions versus Display routines.
* Touch Actions provide a visual feedback to course of where touched.
* Additional items worked out here is the use of palcing Strings in Flash, and later pulled to added to heap. reducing overall heap storage.


Strings to flash and then out to heap this is going to be weird so listen up.

The plan here is to place string liternals into flash memory to save space utilzing pgmspace. and as you may b=have guessed, it is IMMUTABLE. And the simple thought of strcpyu()/strncpy() would be good. **Nope!** need to go a step further.
To do that all string liternal must take the following form:
        `setPatternName((char*)PSTR("Your string here"));`
The reason for this is `<pgmspace.h>` there is the macro definition of `PSTR` which will place the literal in flash
in flash memory. BUT. we need to now cast that to a const char* for the function call.
With that said, the patternname, which we normaly just use `strcpy`, we need to use `strcpy_P()`.
`strcpy_P()` states the string will come from Flashmemory and copy it into SRAM space. 
Once it is in SRAM sapce we can maniputate it all we want. 😀

## To get this up and running - here it folks
Download trhe repot and compile the module place on your Arduino and let er rip.



* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)


