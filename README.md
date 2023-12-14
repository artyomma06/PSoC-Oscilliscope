# PSoC Oscilloscope Project

Welcome to the *PSoC Oscilloscope project*! This program is designed for installation on a Cypress PSoC 6 board, transforming it into a versatile tool for reading and visualizing electrical signals.

## Implementation

Our software employs multiple buffers to efficiently store and process electronic signals. Upon reaching full capacity, an interrupt is triggered. This event prompts the software to scale the buffer values along the x and y axes. Subsequently, the display is cleared, and the newly processed signal is elegantly drawn.

## Prerequisites

To embark on this exciting project, you'll need the following:

- Cypress PSoC 6 board
- Display
- Additional wires to serve as prongs for signal reading
- PSoC Creator for installing the program onto the board

With these prerequisites in place, you'll be all set to explore the capabilities of your PSoC 6 board.

## Usage

This program functions much like a traditional oscilloscope, offering a user-friendly interface to display and interpret electrical signals. Immerse yourself in the world of signal analysis and discover the potential of your PSoC 6 board as an oscilloscope.

*Happy exploring!*
