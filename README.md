# 8play

8play is an unofficial command-line player for [8tracks.com](https://www.8tracks.com).
See `man 8play` for more information.

## Dependencies
curl, ffmpeg, json-c, sdl

## Installation
To install run (as root)
`make clean install`

Note: 8play is installed by default into the /usr/local namespace.
You can change this by specifying `PREFIX`, e.g.
`make PREFIX=/usr intall`.

