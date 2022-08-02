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

You will need to provide your own copy of `doom.wad` for DOOM, `doom2.wad` for DOOM II, `tnt.wad` and `plutonia.wad` for Final DOOM and `SIGIL.wad` (and optionally `SIGIL_SHREDS.wad`) for SIGIL

You can find the file `doom.wad` in any installation of DOOM, available on [Steam](http://store.steampowered.com/app/2280/Ultimate_Doom/), [GOG](https://www.gog.com/game/the_ultimate_doom), and floppy disk from 1-800-IDGAMES (note: do not call 1-800-IDGAMES I don't know where it goes anymore). 

`doom2.wad` is in any installation of DOOM II, available on [Steam](https://store.steampowered.com/app/2300/DOOM_II/) or [GOG](https://www.gog.com/game/doom_ii_final_doom)

`tnt.wad` and `plutonia.wad` are in any installation of Final DOOM, available on [Steam](https://store.steampowered.com/app/2290/Final_DOOM/) or [GOG](https://www.gog.com/game/doom_ii_final_doom) (note that GOG sells DOOM II and Final DOOM packaged together)  

`SIGIL.wad` is available free from the [SIGIL](https://www.romerogames.ie/si6il) website, and `SIGIL_SHREDS.wad` is available in the "registered" (purchased) versions. Note that SIGIL also requires `doom.wad` from the original game. 

This repo contains changes from id's [DOOM-iOS2](https://github.com/id-Software/DOOM-IOS2) repo (different than the parent of this repo), changes from the [FinalJudgement](https://github.com/JadingTsunami/FinalJudgment-iOS) repo by [JadingTsunami](https://github.com/JadingTsunami/), and [MFi controller code](https://github.com/johnnyw/DOOM-IOS2/commit/41646df7ccad6e39263a73767e91d5801759b780) from [John Watson](https://github.com/johnnyw) (by way of [TheRohans](https://github.com/TheRohans/DOOM-IOS2/), where I originally found it). I incorporated the efforts of [yarsrevenge](https://github.com/yarsrvenge/DOOM-IOS2) in getting the basics of the tvOS versions going. 

I wrote a [lengthy article](http://schnapple.com/wolfenstein-3d-and-doom-on-ios-11/) on the process of making these ports. For a rundown of the effort to get it running on tvOS, I wrote a [second lenghty article](http://schnapple.com/wolfenstein-3d-and-doom-on-tvos-for-apple-tv/) on the subject. 

And just for fun I did [another article](https://schnapple.com/sigil-for-ios-and-tvos-for-apple-tv/) on adding SIGIL support. Note that the previous issues with SIGIL's intermission screens and the Buckethead MP3 files have been addressed. 

[Video of DOOM running on an iPhone X](https://www.youtube.com/watch?v=IrY5L1kn-NA)

[Video of DOOM running on an Apple TV](https://www.youtube.com/watch?v=P8QmMSabaqQ)

I have also made apps for [*Wolfenstein 3-D*](https://github.com/tomkidd/Wolf3D-iOS), [*Quake*](https://github.com/tomkidd/Quake-iOS), [*Quake II*](https://github.com/tomkidd/Quake2-iOS), [*Quake III: Arena*](https://github.com/tomkidd/Quake3-iOS), [*Return to Castle Wolfenstein*](https://github.com/tomkidd/RTCW-iOS) and [*DOOM 3*](https://github.com/tomkidd/DOOM3-iOS).

Have fun. For any questions I can be reached at tomkidd@gmail.com
