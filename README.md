<img align="left" width="100" height="100" src="https://raw.githubusercontent.com/tomkidd/DOOM-iOS/master/icon_doom.png">  

#  DOOM, DOOM II, Final DOOM and SIGIL for iOS 11 and tvOS for Apple TV

This is my update for DOOM for iOS to run on iOS 11, running in modern resolutions including the full width of the iPhone X. I have also made a target and version for tvOS to run on Apple TV. Additionally, I have included targets to create apps for DOOM II, Final DOOM and [SIGIL](https://www.romerogames.ie/si6il).

![screenshot](https://raw.githubusercontent.com/tomkidd/DOOM-iOS/master/ss_doom.png)

Improvements/Changes

- Compiles and runs in iOS 11 SDK
- Orientation and coordinate system fixed to reflect iOS 8 changes
- C warnings fixed for Xcode 9.3
- Basic MFi controller support
- Structure and View Controller usage grafted in from the DOOM-iOS2 repository and public user forks, unused code and embedded xcodeproj use eliminated
- Second project target for tvOS that takes advantage of focus model and removes on-screen controls.

This commit uses all new images for the menus and fonts. You will still need to provide your own copy of `doom.wad` for DOOM, `doom2.wad` for DOOM II, `tnt.wad` and `plutonia.wad` for Final DOOM and `SIGIL.wad` (and optionally `SIGIL_SHREDS.wad`) for SIGIL

You can find the file `doom.wad` in any installation of DOOM, available on [Steam](http://store.steampowered.com/app/2280/Ultimate_Doom/), [GOG](https://www.gog.com/game/the_ultimate_doom), and floppy disk from 1-800-IDGAMES (note: do not call 1-800-IDGAMES I don't know where it goes anymore). 

`doom2.wad` is in any installation of DOOM II, available on [Steam](https://store.steampowered.com/app/2300/DOOM_II/) or [GOG](https://www.gog.com/game/doom_ii_final_doom)

`tnt.wad` and `plutonia.wad` are in any installation of Final DOOM, available on [Steam](https://store.steampowered.com/app/2290/Final_DOOM/) or [GOG](https://www.gog.com/game/doom_ii_final_doom) (note that GOG sells DOOM II and Final DOOM packaged together)  

`SIGIL.wad` is available free from the [SIGIL](https://www.romerogames.ie/si6il) website, and `SIGIL_SHREDS.wad` is available in the "registered" (purchased) versions. Note that SIGIL also requires `doom.wad` from the original game. 

This repo contains changes from id's [DOOM-iOS2](https://github.com/id-Software/DOOM-IOS2) repo (different than the parent of this repo), changes from the [FinalJudgement](https://github.com/JadingTsunami/FinalJudgment-iOS) repo by [JadingTsunami](https://github.com/JadingTsunami/), and [MFi controller code](https://github.com/johnnyw/DOOM-IOS2/commit/41646df7ccad6e39263a73767e91d5801759b780) from [John Watson](https://github.com/johnnyw) (by way of [TheRohans](https://github.com/TheRohans/DOOM-IOS2/), where I originally found it). I incorporated the efforts of [yarsrevenge](https://github.com/yarsrvenge/DOOM-IOS2) in getting the basics of the tvOS versions going. 

I wrote a [lengthy article](http://schnapple.com/wolfenstein-3d-and-doom-on-ios-11/) on the process of making these ports. For a rundown of the effort to get it running on tvOS, I wrote a [second lenghty article](http://schnapple.com/wolfenstein-3d-and-doom-on-tvos-for-apple-tv/) on the subject. 

A previous version of this repo required the use of a file called `base.iPack` from an existing copy of the iPhone version of DOOM in  order for the DOOM port to work, but I have now added a "clean room" version of that file and included the resources necessary to build it in case anyone wants to tweak or improve it. Credits for the images used are included below. A third lengthy article on the subject of the base.iPack file and adding the additional two games can be found [here](http://schnapple.com/doom-ii-and-final-doom-for-ios-and-tvos).

And just for fun I did [another article](https://schnapple.com/sigil-for-ios-and-tvos-for-apple-tv/) on adding SIGIL support. Note that the previous issues with SIGIL's intermission screens and the Buckethead MP3 files have been addressed. 

[Video of DOOM running on an iPhone X](https://www.youtube.com/watch?v=IrY5L1kn-NA)

[Video of DOOM running on an Apple TV](https://www.youtube.com/watch?v=P8QmMSabaqQ)

I have also made apps for [*Wolfenstein 3-D*](https://github.com/tomkidd/Wolf3D-iOS), [*Quake*](https://github.com/tomkidd/Quake-iOS), [*Quake II*](https://github.com/tomkidd/Quake2-iOS), [*Quake III: Arena*](https://github.com/tomkidd/Quake3-iOS), [*Return to Castle Wolfenstein*](https://github.com/tomkidd/RTCW-iOS) and [*DOOM 3*](https://github.com/tomkidd/DOOM3-iOS).

Have fun. For any questions I can be reached at tomkidd@gmail.com

---

base.iPack icon and texture credits:

Font texture created with [LMNOpc Font Builder](http://www.lmnopc.com/bitmapfontbuilder/) by Thom Wetzel

Font used is [ChicagoFLF](https://fontlibrary.org/en/font/chicagoflf), public domain

[Control Pad by Guillaume Berry from the Noun Project](https://thenounproject.com/term/control-pad/40359)

[explosion by BomSymbols from the Noun Project](https://thenounproject.com/term/explosion/938854)

[Compass by Adrien Coquet from the Noun Project](https://thenounproject.com/term/compass/1941270)

[Move by useiconic.com from the Noun Project](https://thenounproject.com/term/move/45502)

[turn by shashank singh from the Noun Project](https://thenounproject.com/term/turn/530562)

[drive by priyanka from the Noun Project](https://thenounproject.com/term/drive/1568697)

[Melted Paint](https://publicdomaintextures.wordpress.com/2014/02/14/melted-paint/)

[Plaster in Black and White](https://publicdomaintextures.wordpress.com/2014/04/03/plaster-in-black-and-white/)

[Tool by Ker'is from the Noun Project](https://thenounproject.com/term/tool/1977834)

[circle arrow by Paul Verhulst from the Noun Project](https://thenounproject.com/term/circle-arrow/1979648)

[Save by Markus from the Noun Project](https://thenounproject.com/term/save/1715647)

[Skull by Andrew Cameron from the Noun Project](https://thenounproject.com/term/skull/131075)

[Skull by Andrew Cameron from the Noun Project](https://thenounproject.com/term/skull/131076)



Click sound effects
[1](https://freesound.org/people/EdgardEdition/sounds/113634/)
[2](https://freesound.org/people/dersuperanton/sounds/435845/)
[3](https://freesound.org/people/Eponn/sounds/420997/)
[4](https://freesound.org/people/BehanSean/sounds/422431/)

