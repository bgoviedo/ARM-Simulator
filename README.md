# ARM-Simulator
A C program which is a four-core instruction-level simulator for a 40-instruction subset of the ARMv8 instruction set.

Simulates pipelining as modeled by the classic RISC 5-stage pipeline with support for data dependency handling and control dependency handling. (Instruction Fetch, Instruction Decode, Execute, Memory Access, Register Writeback)

Supports branch prediction using a pattern history table (PHT) and branch target buffer (BTB).

Also simulates L1-caching using a four-way set-associative instruction-cache that is 8 KB in size with 32 byte blocks as well as an eight-way set-associative cache that is 64 KB in size with 32 byte blocks. Uses the Least-Recently-Used (LRU) cache replacement policy.

The instructions supported are:

| ADD   | ADDI  | ADDIS | ADDS | AND  | ANDS  |
|-------|-------|-------|------|------|-------|
| B     | BEQ   | BNE   | BGT  | BLT  | BGE   |
| BLE   | CBNZ  | CBZ   | EOR  | LDUR | LDURB |
| LDURH | LSL   | LSR   | MOVZ | ORR  | STUR  |
| STURB | STURH | STURW | SUB  | SUBI | SUBIS |
| SUBS  | MUL   | SDIV  | UDIV | HLT  | CMP   |
| BL    | BR    |       |      |      |       |

To run, type:

./sim FILE_NAME

The input file should be contain the ARMv8 instructions encoded in hexadecimal format, separated by a newline. The last instruction of this file should be the "hlt 0" instruction, which tells the simulator to halt. This instruction corresponds to the opcode: d4400000. (An example of an input file is given in inputs/fibonacci.x. Its corresponding assembly code is in inputs/fibonacci.s.)

When the executable is run, type "go" to simulate the program until the end. Alternatively, type "run <n>" to simulate the execution of the machine for n clock cycles. At any point in the simulation, type "rdump" to display the contents of the registers, flags, and program counter. Type "mdump <low> <high>" to dump the contents of memory, from location low to location high.
