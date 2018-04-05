#  DOOM for iOS 11

This is my update for DOOM for iOS to run on iOS 11. It also runs in modern resolutions including the full width of the iPhone X.

Improvements/Changes

- Compiles and runs in iOS 11 SDK
- Orientation and coordinate system fixed to reflect iOS 8 changes
- C warnings fixed for Xcode 9.3
- Basic MFi controller support
- Structure and View Controller usage grafted in from the DOOM-iOS2 repository and public user forks, unused code and embedded xcodeproj use eliminated

This commit only includes the changes made to the original source code and the original files. An "IB Images" directory is still required to build correctly, as is a `idGinzaNar-Md2.otf` font file along with `doom.wad` and `base.iPack`, but as those consist of copyrighted material, I have not included them in this commit. 

My plan is to do a pull request to id Software on this commit and then make a second commit with placeholder menu art and font for others to be able to compile once they provide a `doom.wad` file and `base.iPack` file. The `doom.wad` file can be found in any installation of DOOM but the `base.iPack` file must be sourced from elsewhere or created with a utility that I have yet to write.