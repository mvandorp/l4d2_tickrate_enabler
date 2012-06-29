tickrate_enabler 1.0 by ProdigySim

======================
Description
======================

Enables the "-tickrate" switch to be used from the command line to set the game's tickrate.

Patches Boomer Vomit behavior to fix an issue where vomit range scaled inversely with tickrate.

======================
Instructions
======================

1. Place tickrate_enabler.dll (Windows) or tickrate_enabler.so (Linux) in your server's addons folder.
2. Place tickrate_enabler.vdf (L4D2) or tickrate_enabler_l4d1.vdf (L4D1) in your server's addons folder.
3. Add "-tickrate <desired_tickrate>" to your server's launch parameters. e.g. -tickrate 100
4. Make sure the following convar settings are properly set in server.cfg or otherwise:

    sv_maxupdaterate 100
    sv_maxcmdrate 100
    fps_max 150 // higher than 100 recommended, as ticks calculated seems to dip otherwise.

5. Done. Enjoy 100 tick L4D2 gameplay.

======================
Changelog / TODO
======================

1.0:
    Patches boomer vomit to behave properly on modified high tickrates
    Code cleaner perhaps
0.1: 
    -tickrate only pseudo-release

TODO:
	Max rate patches

======================
Credits / License
======================
tickrate_enabler is copyright Michael "ProdigySim" Busby 2012

Parts of this project contain GPLv3 code adapted from SourceMod (Allied Modders L.L.C.) and Left4Downtown2 (Igor Smirnov et. al.).

This entire project is released under the AlliedModders modified GPLv3.
