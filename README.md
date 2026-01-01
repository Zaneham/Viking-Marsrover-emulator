# DDP-24 Emulator

An emulator for the computer that landed on Mars. Twice.

## What Is This?

This is an emulator for the DDP-24, the guidance computer used in the Viking Mars Landers. Viking 1 touched down on July 20, 1976. Viking 2 followed on September 3, 1976. They were the first American spacecraft to successfully land on Mars and send back pictures of what it actually looked like. Spoiler: red. Very red.

The computers that made this happen were built by Computer Control Company of Framingham, Massachusetts. The instruction manual is dated August 1964. The Vikings launched in 1975. NASA had over a decade to write the software. They used every minute of it.

The DDP-24 used 24-bit words because apparently that was the correct number of bits in 1964. Sign-magnitude arithmetic, because floating point was for people who didn't know what they were doing. 32K words of magnetic core memory, because that was enough for anybody.

The landers worked perfectly. For years. On Mars. 200 million kilometres from the nearest engineer. We thought someone should probably understand how they did that.

## Features

- **Full DDP-24 instruction set** (well, most of it) (we're working on it)
- **24-bit sign-magnitude arithmetic** (because two's complement hadn't been invented yet) (it had, they just didn't use it)
- **Index registers** (three of them, plus one that's always zero, because that's useful apparently)
- **Indirect addressing** (for when direct addressing just isn't complicated enough)
- **Interactive debugger** (the original engineers had front panel switches, you get a command line)

## Architecture

```
Word Size:       24 bits (sign-magnitude, naturally)
Memory:          32K words (15-bit address space)
Registers:       A, B (accumulators), X1-X3 (index)
Clock:           ~100,000 instructions/second (on the original hardware)
                 Considerably faster now. Moore's Law and all that.
Weight:          You can't lift it. Don't try.
Power:           More than your laptop. Less than your kettle.
```

## Building

```bash
make
```

If that doesn't work, you'll need a C compiler. We recommend any compiler from after 1976. If you're using something older than the Viking mission itself, I have questions.

## Usage

```bash
# Run the test suite
./ddp24 -t

# Run a program
./ddp24 program.bin

# Interactive mode
./ddp24 -i program.bin

# Run and dump state after
./ddp24 -d program.bin
```

### Interactive Commands

| Command | Description |
|---------|-------------|
| `s` | Step one instruction |
| `r` | Run until halt |
| `d` | Dump CPU state |
| `m <addr>` | Show memory at octal address |
| `q` | Quit (also works on Mars, presumably) |

## Instruction Format

```
┌─────────────────────────────────────────────────────┐
│  Opcode  │ I │  X  │         Address               │
│  (6 bits)│(1)│ (2) │        (15 bits)              │
└─────────────────────────────────────────────────────┘
    23-18   17  16-15            14-0
```

The I bit enables indirect addressing. The X field selects an index register. X=0 means no indexing (because X0 is hardwired to zero, which is either elegant or annoying depending on your perspective).

## Documentation Sources

This emulator was built from original documentation preserved in various archives and attics:

- **DDP-24 Instruction Manual** (August 1964), Computer Control Company, Inc.
- **DDP-124 Programmer's Reference Manual** (August 1965)
- **DDP-24 Programming Card** (the wallet-sized reference for when you're debugging on the go)

Computer Control Company was later acquired by Honeywell. The DDP-24 became the Honeywell 316 and its descendants. The documentation survived. The company did not.

## Why This Matters

Viking was the first successful Mars landing mission. The landers operated for years, far beyond their design life. Viking 1 kept transmitting until 1982. Viking 2 made it to 1980. The computers never failed.

The engineers who built these systems did so with tools that would seem primitive today: paper tape, magnetic core, and a lot of very careful testing. They got it right. So right that their work kept operating for six years on another planet with a 20-minute communication delay and no possibility of repair.

Understanding how they did that isn't just historical curiosity. It's a masterclass in engineering under constraints. We have faster computers now. We're not sure we're better at using them.

## Known Limitations

- I/O instructions are stubbed (your emulated Mars lander cannot actually phone home)
- Cycle timing is approximate (but your code will run, which is the main thing)
- Cannot actually land on Mars (this is a limitation of your hardware, not our software)

## Related Projects

If you've enjoyed emulating the computers that landed on Mars, you might appreciate:

- **[Minuteman Guidance Computer Emulator](https://github.com/Zaneham/minuteman-emu)** (the D17B/D37C that guides ICBMs. Same era, different trajectory. The Minuteman goes up; the Viking went sideways and then down. Both worked.)

- **[Voyager Emulator](coming soon)** (the computers currently leaving the solar system. We're waiting on documentation from a university in Kansas. NASA lost their copies. We are not making this up.)

- **[HAL/S LSP](https://github.com/Zaneham/hals-lsp)** (language server for the Space Shuttle. The Shuttle came later but borrowed a lot of ideas. Mostly "don't crash.")

## Licence

Apache 2.0. See [LICENSE](LICENSE) for details.

## Contact

Questions? Found a bug? Have original Viking flight software sitting in a filing cabinet somewhere?

zanehambly@gmail.com

Response time faster than a round-trip signal to Mars. Which is somewhere between 8 and 48 minutes depending on orbital positions. So, functionally, immediately.

Available to discuss Martian computers over coffee. Will explain why 24-bit words made sense in 1964. Has opinions about sign-magnitude arithmetic but will keep them to himself if you're buying.

---

*"The Viking landers transmitted from Mars for six years. Your laptop crashes if you open too many browser tabs. Progress is complicated."*
