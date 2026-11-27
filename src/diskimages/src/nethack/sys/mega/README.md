# NetHack 3.6.7 for the Mega-Micros SBC-3

This is NetHack 3.6.7 as released, built for the SBC-3 and MicroMon with
the tty interface on a VT100/ANSI serial terminal of 80x24.

## Playing

The build puts the game on the `tools` disk image:

    bin/nethack              the game
    games/nethack/           its data, saved games, scores and lock files

Run it with MicroMon's `run` command, by its full path:

    run /<disk>/bin/nethack

The game finds its data from the path it was run by: `/<disk>/bin/nethack`
uses `/<disk>/games/nethack`, whatever `<disk>` is mounted as. Give it
`-d <dir>` to use another directory. The disk has to be writable, for
saved games and the score list.

The terminal should be VT100 or ANSI compatible, with 80 columns and 24
lines. Your options go in `games/nethack/.nethackrc`, as on Unix: for
example `OPTIONS=DECgraphics` draws the map with line-drawing characters,
and `OPTIONS=color` turns on colour.

`nethack -D` starts in debug (wizard) mode; `games/nethack/sysconf` says
who may use it.

## Building

The game's data (compiled levels, the dungeon, help texts...) is made
on the PC by NetHack's own utilities, built for the 68k as Linux programs
and run under `qemu-m68k`, so what they write is laid out just as the
game on the board reads it. That needs Debian's `qemu-user` package (and
the cross compiler's C library, `libc6-dev-m68k-cross`). Without
`qemu-m68k`, the build leaves NetHack out and says so.

## What's changed from the release

Only one of NetHack's own files: `include/unixconf.h`, which includes
`include/megaconf.h` (the board's configuration) in place of its choice
of Unix flavour. The rest of the port is here, in `sys/mega/`:

- `megamain.c` - `main()`: sets up the environment MicroMon doesn't
  provide (where the game's files are, the player, the terminal), then
  runs NetHack's Unix `main()` from `sys/unix/unixmain.c`
- `megatty.c` - the terminal, in place of `sys/share/unixtty.c` and the
  termcap library: a built-in VT100 terminal description
- `megacompat.c` - Unix calls the board has no use for (users, hard
  links, child processes)
- `include/` - `pwd.h` and `sys/wait.h`, for those
- `sysconf` - the system-wide settings

The game and its data are as on any Unix: lock files are made with
`fcntl()` (always granted, as only one program runs at a time), and saved
games aren't compressed. There's no `recover` utility: a game left by a
crash or power cut is lost (its level files, `<n>lock.*`, can be deleted).

Only the parts of the NetHack source the build uses are here: `src`,
`include`, `util`, `dat`, `win/tty` and the few files of `sys/share` and
`sys/unix` it needs.
