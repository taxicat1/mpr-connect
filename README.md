# mpr-connect

Connect a retail cartridge of Pokémon Diamond, Pearl, or Platinum to My Pokémon Ranch. Importantly, this bypasses the RSA signature check on the downloaded ROM file, allowing for modded clients to be run.

This requires the Pokémon cartridge to be in Slot-1 of the console while the program is running. To do this, either:
- Use a CFW 3DS/2DS and launch mpr-connect via TWiLight Menu++
- Use a modded DSi and launch mpr-connect via TWiLight Menu++ through Unlaunch
- Use any Slot-1 flashcart (R4 etc.) to launch mpr-connect, eject the flashcart, and then insert the Pokémon cartridge

## Building

This is a BlocksDS homebrew ROM. Install [BlocksDS](https://blocksds.skylyrac.net/docs/setup/), then it can be built by running `make` through Wonderful Toolchain.
