# Bugs
- [X] Crash when disconnect
- [X] Crash on reconnect

- Memory Widget
   - [X] Watch type "float" not highlighted correctly
   - [X] Editing text does not work
   - [X] Search
         - [X] if there's only one instance of data, find-next and find-prev return "not found"
         - [X] Text elem should have foces when find group is opened
         - [X] When switching from hex to text or the other way round, search results are not cleared   

- Watches Widget
   - [X] Strings not PETSCII encoded?
   - [ ] Tree branch controls are shown

- Breakpoint Widget
   - [ ] Tree branch controls are shown

- Disassembly:
   - [X] Scrollbars not disabled at startup

# TODO

- [ ] Label support
      - [ ] in "Go to address" dropdown
      - [ ] in "add breakpoint" dialog
      - [ ] in "add watch" dialog

## Disassembler Widget
- [ ] Toolbar
      - [X] "Jump to address" box"
      - [X] "Jump to PC"
      - [ ] Load labels
- [ ] Context Menu for all toolbar actions
- [X] Make sure line is visible
- [ ] Mouse-over breakpoints shows breakpoint info
- [ ] Context Menu on address params to create watchpoints
- [X] Switch between 6502 and Z80

## Memory Widget
- [X] Allow modifying memory
- [X] Toolbar
      - [ ] "Jump to address" box
      - [ ] switch between uc/graphics and lc/uc fonts (U+EExx vs U+EFxx)
      - [X] allow to select bank
- [X] Support different banks
- [X] Highlight read/write breakpoints
- [X] Highlight watches
- [X] Mouse-over addresses with breakpoints shows breakpoint info
- [X] Mouse-over addresses with watches shows watch info
- [X] Context-Menu that allows:
      - [X] add a watch
      - [X] add a breakpoint
- [ ] Search
      - [ ] hex
      - [ ] text

## Breakpoints Widget
- [X] Add breakpoint
- [X] Remove breakpoint
- [X] Enable/Disable breakpoints
- [ ] Context menu
      - [ ] modify
      - [ ] delete

## Watches Widget
- [X] Add
- [X] Remove
- [X] Edit
- [X] Different view types (int, uint, hex, text)
- [X] Remove "int hex" variants (they are the same as uint hex)
- [X] Support banks
      - [X] in Watches Dialog
      - [X] in Watches
- [ ] Context menu
      - [ ] modify
      - [ ] delete
      - [ ] Over value, allow to select presentation

## Registers Widget
- [X] Add register modification

## Other stuff
- [ ] Derive system we're debugging from VICE resopnses
- [X] Use different colors for icons in toolbar menu
- [ ] Add "Connect" and "Quick connect" menu
- [ ] Add About box
- [ ] add sprite viewer (all sprites, currently active sprites)
- [ ] Font viewer (currently installed font)
- [ ] Screen viewer (currently active screen)
- [ ] Support Z80 for Commodore 128
      - [X] Disassembler
      - [ ] Figure out when Z80 is active
- [ ] Support CPU jams

## Make it look good
### Windows
- [ ] Font size for watches and breakpoints
