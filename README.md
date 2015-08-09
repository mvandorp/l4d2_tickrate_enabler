Tickrate Enabler by ProdigySim
==============
Enables the "-tickrate" switch to be used from the command line to set the game's tickrate.

Patches Boomer Vomit behavior to fix an issue where vomit range scaled inversely with tickrate.

Removes global upper-limits on the max client data rate (was 30000),
and removes the (30k) limits on rate control cvars:
- sv_maxrate
- sv_minrate
- net_splitpacket_maxrate

Build instructions
--------------
First clone the repository to a directory of your liking to obtain the source code:

        git clone https://github.com/mvandorp/l4d2_tickrate_enabler.git

Modify *Makefile* so that the SRCDS, HL2SDK_L4D, HL2SDK_L4D2 and MMSOURCE paths are correct for your environment.
Then, from inside the l4d2_tickrate_enabler directory, run *make* to build the project:

        cd l4d2_tickrate_enabler
        make

This will compile the project and also create a tickrate_enabler.tar.gz containing the files necessary for installation.

Install
--------------
You can use either tickrate_enabler.tar.gz that you obtained from building the project with the instructions above, or you can download it from the releases on the GitHub repository.

Then simply extract the archive to your server's 'left4dead2' directory to install it.

Credits
--------------
tickrate_enabler is copyright Michael "ProdigySim" Busby 2012

Parts of this project contain GPLv3 code adapted from SourceMod (Allied Modders L.L.C.) and Left4Downtown2 (Igor Smirnov et. al.).

This entire project is released under the AlliedModders modified GPLv3.