# TFT Test Driver: #

TFT Test Driver is a mini project I used to help me define how to use the TFT touch screeen for controls and display


### What is this repository for? ###

* Provides a boiler plate method for headers
* Provides a repeatable for Screen displays
* Provides a seperation of Touch actions versus Display routines.
* Touch Actions provide a visual feedback to course of where touched.
* Additional items worked out here is the use of palcing Strings in Flash, and later pulled to added to heap. reducing overall heap storage.

There is a few items I wanted to accomplish in this project.
* Obviously, I wanted to work on a TFT understand its interfaces - and of course touch controls.
* At first the items began to seem repetative.  So Looked for ways to boiler plate a few items. for my screens anyway.👀
* Memory, Memory and oh yeah not alot of memory

One of the biggest items that is rather confining is memory, especially in embedded systems and simply there is no just add more memory mentality.  Global strings take up a chunck of memory,  String literals take up more - ughh! **So I set out to find a way to reduce all the bloat that is caused by the string literals**

At first I though about the String class, yeah no good.  because of the manner in the class works it will fragment memory, to a point where the application simply dies. #NoBueno!

I also though about an array, of character and index. well ok, helped only very little.


# Striking Gold - Flash the strings...

Strings placed to flash and then out to heap was the wake up call.  There is not a whole lot of explanation, so I will try to make this easy.  this is going to be weird so listen up.

The plan here is to place string literals into flash memory to save space utilzing **`pgmspace`**. and as you may have guessed, it is IMMUTABLE. And the simple thought of strcpy()/strncpy() would be good. **Nope!** yet another #NoBueno!  Need to go a step further.

To do that all the string literals of interest for this feature  must take the following form:
        `(char*)PSTR("Your string here");` . for example:
        `setPatternName((char*)PSTR("Your string here"));`
The reason for this is `<pgmspace.h>` there is the macro definition of `PSTR` which will place the literal in flash
in flash memory. BUT. we need to now cast that to a const char* for the function call.
With that said, the patternname, which we normaly just use `strcpy`, we need to use `strcpy_P()`.
`strcpy_P()` states the string will come from Flashmemory and copy it into SRAM space. 
Once it is in SRAM sapce we can maniputate it all we want. 😀

One note:  If you are using the `Serial.print()` and `Serial.println()`  There is a also simple solution here ...Thats right **F()** your strings. For example: `Serial.println(F("My String literal here"));`  Every little bit helps.



## To get this up and running - here it is folks:
Download this repo, open the sketch, compile and place the executable module on your Arduino... Let 'er rip.



