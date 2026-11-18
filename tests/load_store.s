# Loads y stores de byte/halfword: muestra el orden little-endian
# y la diferencia entre loads con signo (lb, lh) y sin signo (lbu, lhu).
.global _start
_start:
    li   t0, 0x1000
    li   t1, 0x12345678
    sw   t1, 0(t0)      # mem[0x1000..0x1003] = 78 56 34 12
    lb   a0, 0(t0)      # a0 = 0x00000078  (byte menos significativo)
    lb   a1, 3(t0)      # a1 = 0x00000012  (byte mas significativo)
    lh   a2, 2(t0)      # a2 = 0x00001234
    li   t2, -2         # t2 = 0xFFFFFFFE
    sh   t2, 4(t0)      # mem[0x1004..0x1005] = FE FF
    sb   t2, 6(t0)      # mem[0x1006] = FE
    lh   a3, 4(t0)      # a3 = 0xFFFFFFFE  (con signo)
    lhu  a4, 4(t0)      # a4 = 0x0000FFFE  (sin signo)
    lb   a5, 6(t0)      # a5 = 0xFFFFFFFE  (con signo)
    lbu  a6, 6(t0)      # a6 = 0x000000FE  (sin signo)
fin:
    j    fin
