# Simulador RISC-V (RV32I)

Simulador del ISA base RV32I de RISC-V en C++. Carga un programa ya compilado
(binario crudo) en la dirección `0x00000000` y lo ejecuta paso a paso,
permitiendo inspeccionar el PC, los registros y la memoria.

## Compilar

Con CMake:

```
cmake -B build
cmake --build build
```

O directamente con g++:

```
g++ -std=c++17 -O2 -o build/riscv-simulator src/main.cpp
```

## Uso

```
./build/riscv-simulator programa.bin
```

| Comando | Descripción |
|---|---|
| `step [n]` | Ejecuta `n` instrucciones (por defecto 1) |
| `pc` | Muestra el PC |
| `regs [r1 r2 ...]` | Muestra registros (`x0`–`x31` o nombres ABI como `a0`, `sp`); sin argumentos muestra los 32 |
| `mem <inicio> [fin]` | Muestra los bytes de memoria de `inicio` a `fin` (inclusive) |
| `help` | Muestra la ayuda |
| `exit` | Sale del simulador |

Ejemplo:

```
$ ./build/riscv-simulator tests/riscvtest.bin
"tests/riscvtest.bin" cargado a memoria. Escribe 'help' para ver los comandos.
> step 100
Ejecutadas 100 instruccion(es). pc = 0x00000050
> mem 0x64 0x67
0x00000064: 19 00 00 00
```

## Programas de prueba (`tests/`)

| Programa | Resultado esperado |
|---|---|
| `riscvtest.bin` | Escribe 25 (`0x19`) en la dirección `0x64` |
| `quicksort.bin` | Deja `{1, 2, 3, 4, 6, 8, 9}` en `0x1000` |
| `arbol_simetrico.bin` | Termina con `a0 = 1` |
| `load_store.bin` | Loads/stores de byte y halfword (little-endian, con y sin signo) |

Cada `.bin` es un binario crudo generado a partir del `.s` del mismo nombre.
