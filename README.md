# elem_arithmetic_compiler

Front end for a compiler into three address code intermediate representation for the language defined by the following CFG:


    S -> 𝓵=A
    A -> 𝓵B
    B -> αA | ɛ
    𝓵 -> any single letter from the alphabet
    α -> + | - | * | /
