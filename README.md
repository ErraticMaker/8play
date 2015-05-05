# 8play

8play is an unofficial command-line player for [8tracks.com](https://www.8tracks.com).
See `man 8play` or [8play(1)](https://jgmp.github.io/8play.html) for more information.

8play differs quite a bit from [8p](https://github.com/jgmp/8p).  I dropped the
dependencies for VLC and ncurses.  I wrote my own player library based on
FFmpeg and SDL ([libplayer](https://github.com/jgmp/libplayer)), making 8play
more lightweight.

## Screenshot
![Screenshot](screenshot.png?raw=true)

## Dependencies
curl, ffmpeg, json-c, sdl

## Installation
First clone the repository (including the submodule libplayer).  
`git clone --recursive https://github.com/jgmp/8play.git`

To install run (as root)  
`make clean install`

Note: 8play is installed by default into the /usr/local namespace.
You can change this by specifying `PREFIX`, e.g.  
`make PREFIX=/usr intall`.

### Arch Linux

[AUR Package](https://aur.archlinux.org/packages/8play)

