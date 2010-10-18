Open Manager v1.0
-----------------

This application has been made by an anonymous person non seeking fame, profit or benefit.

I don´t want to cause any damage to the video game industry, but it is not reasonable that these consoles (expensive cost) are constructed like butter and we can not do anything to preserve the health of the lens of our readers (my console model cannot be repaired), While Sony is rubbing his hands when you send to repair a console and them are dedicated to eliminating original features of the console, as PS3 Other OS and Linux, relying on what they think best for your business and not caring a shit or the feelings of their users or their acquired rights as users and owners of their consoles.

 The videogame industry only wants control, oligopoly, money, pursuit of anything that does not benefit their interests and if it is possible, force legal changes using their influence and power while we're just anonymous people we have to endure all the crap on hardware that is passed through a computer, just to avoid taxes.

So I decided to do an experiment: I decided to see if I can make a BACKUP and Homebrew purpose loader (legal in my country), release the source code and waits to see what happens. Maybe someone has the Sony SDK 1.92 and wants to compile, it but that's not my problem.

Obviously, is 'illegal' to build things with the SONY SDK unlicensed but this is only source code ;)


FEATURES
--------

Open Manager has the next features:

- Supports up to 512 items from two USB devices, internal hdd and bluray

- Ability to copy games from bdvd to hdd0 or USB's, from hdd0 to USB's or from USB's to hdd0. It is done asking to you the destination device. It use uses asynchronous reading and writing for fast copy.

- Special support for big files in FAT: it splits files >= 4GB using .666xx fragments (if the file is name.ext, the file splits as name.ext.66600, name.ext.66601,...). This file is not bootable and it is marked using '_' as folder prefix. The file is joining when you copy it to hdd0 device

- Automatically finds the path of the games and asks to you about it. In other case, if the Open Manager is installed in the hdd0 creates one. You can change it pressing L2+START later.

- Including checking game option (pressing L1). It displays information about the files, size, splits or big files, if it have one.

- Support for Homebrew: you can creates "homebrew" folder in root USB FAT device and put here others folders with an EBOOT.BIN (.self format) and one ICON0.PNG. Also you can use the internal hdd0. Homebrew is launched directly and receive the path in argv[0].

- FTP support to upload homebrew: open_manager uses "OMAN46756" as user, by default. I recommend flashfxp to use it, because FTP support is not very good...

NOTE: Open Manager works with Hermes payload update. So,if you return from game list, apps_home can launch gamaes without one disc (some games don´t work without any disc), but if you return from Homebrew, apps_home works from usb000

NOTE2: You can modify the program as you want, but please, make public the source code




V1.1 changelog
----------------------
- Added the controller fix with the help of hermes, but he didn't know that his fix could screw some other games.
- If you press R2, you will toggle the controller fix.

Important notice: You have to toggle into normal mode to load those games with EBOOT.BIN problem.

Important notice: Only games with controller issues should be played in patched mode

Important notice: A payload with peek/poke calls is mandatory. (hermesV1.0 or PSGroove v1.1)

Important notice: Hermes v2.0 will create compatibility problems, do not use it. It's confirmed that it's broken.

Important notice: Hermes v1.0 is recommended.

known bugs
------------
- Game name may not show correctly, you can reporduce this by enabling patch mode and restarting Open Manager v1.1( I will fix this shortly in v1.12)

moh.sakhaii@psx-scene


Also an indicator for normal and patched mode will be available in version 1.12.

Games that are fixed with Open Manager v1.1 patched mode
-------------------------------------------------
1- Street fighter 4.
2- Super street fighter 4.
3- Eye Pet (Move game)
4- F1 2010 (everybody here eventually got it working )
5- NBA 2K11.
6- White Album (A Japanese visual novel/game)
7- Racket Sports (Move game)
8- Mortal combat (Move game)


v1.12 changelog
--------------------
- Added "patched mode"/"Normal mode" labels to the top-right corner of the game-title screen.

- game names will always show correctly after switching to "Normal mode".



v1.13 changelog
--------------------
- The first state of the label is now corrected, as it was showing normal mode on start.



For compiling the package my friend jtanner created a guide for installing the SDK (windows) and then compiling the Open Manager, thank you james
It is available here: http://jamestanner.co.uk/ps3/

v1.14 changelog
-------------------
It's a minor update
- Corrected the spelling and grammar problems of the first release thanks to DJLO
- changed the icon to what is provided by Iroot
- The open manager termination will be done without crashing if user presses the PS button and selects the “Quit Game” menu.

v1.14.2 changelog
------------------
Another minor update
- You can now use left arrow and right arrow for page-down and page-up functionality in game list.
- Free space will be shown below each device and notice that depending on the amount the unit will change to GB,MB,KB or B.

v1.15 changelog
-------------------
- Removed that patch mode as it is no longer needed.

v1.16 changelog
-----------------
- patched mode returns and now works for hermes V3 and previous versions that include peek/poke calls (v > psgroove 1.1)

Notes: patched mode in hermes V3 is an aggressive mode for those of you who still have problems with hermes v3 normal mode, again do not use patched mode in normal games without issue or for normal operations like FTP/COPY.

v1.16.2 changelog
------------------

- Device names are now changed to more user friendly ones.

v1.17 changelog
----------------

- Open manager has been built with the new SDK 3.40, Note that unlike some others that couldn't figure out how to build it with the libftp, it now builds with 3.40 without any problem.

- PL3 is now fully supported and syscall35 is used although no compatibility is gained from this.

- Previous payloads are still supported (PSGroove > 1.1), open manager detects your current payload and acts accordingly, therefore patched mode is still available for older payloads.


v1.17.1 changelog
-----------------

- Reverted back to using SDK 1.92. There was two reasons for this. First to make it compatible with older firmwares like 3.15 and second, there is no reason to update the SDK just yet.

- mounting to app_home now works without problem. 
