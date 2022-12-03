# elem_arithmetic_compiler

Front end for a compiler into three address code intermediate representation for the language defined by the following CFG:


    S -> 𝓵=A
    A -> 𝓵B
    B -> αA | ɛ
    𝓵 -> any single letter from the alphabet
    α -> + | - | * | /
    
Might add backend for a RISCV 32VI ISA processor supporting a subset of all its instructions later, accounting for the lack of certain data/branching/stalling hazard handling present in the naive multistage pipelined processor architecture I made
