# README

## Intro

`vicedebug` is a debugger frontend for VICE that uses the binary debugger 
protocol. It can be used with any system that VICE can emulate (except 
SuperPET with a 6809 CPU), but in all honesty, I only care about C128
debugability :-) It *does* support the Z80 CPU, and automatically detects
what CPU is active.

`vicedebug` was used to debug a bunch of small C128 intros, but it 
is far from complete. If you end up in a weird situation, try to
disconnect/reconnect. Also, please file a bug report/feature request
on [https://github.com/asig/vicedebug](https://github.com/asig/vicedebug)!

Although `vicedebug` will run on any platform where Qt is available,
all the development happened under Ubuntu. *Some* testing was done under
Windows, but besides that, *no* time was spent trying to make it work
or even look good on any other platform. 

## Building

### Installing prerequisites
Assuming you're on Ubuntu or Debian, run this command:

```bash
sudo apt-get install cmake qt6-base-dev qt6-gtk-platformtheme adwaita-qt6
```

If you want to modify the code, I suggest you also install QtCreator:

```bash
sudo apt-get install qtcreator
```

On other systems, run the equivalent commands that give you a dev
environment.

### Building the binary

On the command line: `cmake . -B build && cd build && make -j$(nproc)`

Of course, you can also just open project in QtCreator and build it from there.
On Windows, this is in my opinion by far the easiest approach.


## Running `vicedebug`
1. Make sure that you have an VICE emulator running that was started with the `-binarymonitor` flag.
2. Execute `./vicedebug` 
3. Click the "connect" button (top left in the toolbar)
4. Enjoy! 

## Using `vicedebug`

### The main window
TODO

### Disassembly view
TODO

### Memory view
TODO

### Breakpoints
TODO

### Watches
TODO

### Other tools
TODO

## License
Copyright (c) 2024 Andreas Signer.  
Licensed under [GPLv3](https://www.gnu.org/licenses/gpl-3.0).

Bug in "About" logo by Sidney Vincent from [Noun Project](https://thenounproject.com/browse/icons/term/insect/) (CC BY 3.0)
