ALttP Randomizer Bot
====================

The goal is to write a bot that can play [_Legend of Zelda: A Link to the Past_'s Randomizer](https://alttpr.com/en) in a similar way that a human would.

Unlike a Tool-Assisted Speedrun (_TAS_) of the original game, the _Randomizer_ shuffles the locations of items around, so the bot can't blindly follow a script verbatim.

The bot reads out chunks of memory from the emulated SNES to understand what is on-screen: it's not operating on the video/images shown on screen. For fairness, it doesn't use any data that wouldn't be knowable to a human player: e.g. it doesn't peak inside chests before opening them.

Architecture
------------

The bot is written as a library that gets linked into a slightly-modified [Snes9x](https://github.com/snes9xgit/snes9x) emulator. There are 2 key functions in the interface:

- `void ap_tick(uint32_t frame, uint16_t * joypad)`
    - This function is called each frame, and blocks emulation until the bot returns. The bot can set the controller state with the `*joypad` outparameter.
- `uint8_t * (*base)(uint32_t addr)` on `struct ap_snes9x`
    - This function can be used by the bot to get the memory address for a given SNES virtual memory address.

The overall code quality is pretty bad, and is mostly shaped around rapid iteration.


Videos in action
----------------

[All media](https://github.com/zbanks/alttp/tree/media)

- [24 minutes of gameplay, from 2020/10/04, sped up 10x](https://github.com/zbanks/alttp/blob/media/alttp_20200104_10x.mp4)
    - Randomizer Settings: Open mode, Sword on Uncle, No Glitches, Defeat Ganon
    - Cheating: infinite health, bombs, and arrows
    - Hits about ~45 checks, clears Eastern Palace

Credits
-------

This wouldn't be possible without the crazy amount of reverse-engineering effort put into this game. 

- MathOnNapkin's Zelda 3 RAM/ROM/SRAM documentation
- MathOnNapkin's Zelda 3 Annotated Disassembly
- wiiqwertyuiop's Zelda 3 Annotated Disassembly
