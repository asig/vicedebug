# TODO

## Disassembler Widget
- [ ] "Jump to address" box, "Jump to pc"
- [X] Make sure line is visible
- [ ] Mouse-over breakpoints shows breakpoint info

## Memory Widget
- [X] Allow modifying memory
- [X] Toolbar
      - [ ] "Jump to address" box
      - [ ] switch between uc/graphics and lc/uc fonts (U+EExx vs U+EFxx)
      - [X] allow to select bank
- [X] Support different banks
- [ ] Highlight read/write breakpoints
- [ ] Context-Menu that allows:
      - [ ] add a watch
      - [ ] add a breakpoint

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

## Make it look good
### Windows
- [ ] Font size for watches and breakpoints
