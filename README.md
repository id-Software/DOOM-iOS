#  DOOM for iOS 11

This is my update for DOOM for iOS to run on iOS 11. It also runs in modern resolutions including the full width of the iPhone X.

Improvements/Changes

- Compiles and runs in iOS 11 SDK
- Orientation and coordinate system fixed to reflect iOS 8 changes
- C warnings fixed for Xcode 9.3
- Basic MFi controller support
- Structure and View Controller usage grafted in from the DOOM-iOS2 repository and public user forks, unused code and embedded xcodeproj use eliminated

This commit adds placeholder files for the "IB Images" folder and the `idGinzaNar-Md2.otf` font file. You will still need to provide your own copies of `doom.wad` and `base.iPack`. 

You can find the file `doom.wad` in any installation of DOOM, available on [Steam](http://store.steampowered.com/app/2280/Ultimate_Doom/), [GOG](https://www.gog.com/game/the_ultimate_doom), and floppy disk from 1-800-IDGAMES (note: do not call 1-800-IDGAMES I don't know where it goes anymore). 

The file `base.iPack` is not included in any DOOM installation and is specific to the iOS port. I can't include it in this repo because it contains copyrighted material and I can't tell you where to find it either, but you will need to source it yourself. The history is included in this [lengthy article](http://schnapple.com/wolfenstein-3d-and-doom-on-ios-11/) I wrote on the subject. At some point I hope to have a utility that will let you construct a `base.iPack` file but for now I don't. 

This repo contains changes from id's [DOOM-iOS2](https://github.com/id-Software/DOOM-IOS2) repo (different than the parent of this repo), changes from the [FinalJudgement](https://github.com/JadingTsunami/FinalJudgment-iOS) repo by [JadingTsunami](https://github.com/JadingTsunami/), and [MFi controller code](https://github.com/TheRohans/DOOM-IOS2/commit/5a6b69d5e9821134f4013b069faef29190dcd7a1) from [TheRohans](https://github.com/TheRohans/).

Have fun. For any questions I can be reached at tomkidd@gmail.com