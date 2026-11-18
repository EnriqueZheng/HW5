# Quicksort in-place recursivo (particion de Lomuto, pivote = arr[r-1]).
# Coloca {6, 4, 3, 2, 1, 8, 9} en 0x1000 y lo deja ordenado: {1, 2, 3, 4, 6, 8, 9}.
.global _start
_start:
    li   sp, 0x10000
    li   a0, 0x1000
    li   t0, 6
    sw   t0, 0(a0)
    li   t0, 4
    sw   t0, 4(a0)
    li   t0, 3
    sw   t0, 8(a0)
    li   t0, 2
    sw   t0, 12(a0)
    li   t0, 1
    sw   t0, 16(a0)
    li   t0, 8
    sw   t0, 20(a0)
    li   t0, 9
    sw   t0, 24(a0)
    li   a1, 0          # l = 0
    li   a2, 7          # r = 7 (exclusivo)
    call _qsort
_end:
    j    _end

# _qsort(a0 = arr, a1 = l, a2 = r)  ordena arr[l..r-1]
_qsort:
    bge  a1, a2, qs_ret
    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   s0, 8(sp)
    sw   s1, 4(sp)
    sw   s2, 0(sp)
    mv   s0, a0
    mv   s1, a1
    mv   s2, a2
    call _partition     # a0 <- indice final del pivote
    mv   t0, a0
    mv   a0, s0         # _qsort(arr, l, p)
    mv   a1, s1
    mv   a2, t0
    mv   s1, t0         # s1 <- p
    call _qsort
    mv   a0, s0         # _qsort(arr, p + 1, r)
    addi a1, s1, 1
    mv   a2, s2
    call _qsort
    lw   ra, 12(sp)
    lw   s0, 8(sp)
    lw   s1, 4(sp)
    lw   s2, 0(sp)
    addi sp, sp, 16
qs_ret:
    ret

# _partition(a0 = arr, a1 = l, a2 = r) -> a0 = indice del pivote
_partition:
    addi t0, a2, -1
    slli t0, t0, 2
    add  t0, t0, a0
    lw   t1, 0(t0)      # t1 <- pivote = arr[r-1]
    mv   t2, a1         # i = l
    mv   t3, a1         # j = l
pt_loop:
    bge  t3, a2, pt_done
    slli t4, t3, 2
    add  t4, t4, a0
    lw   t5, 0(t4)      # t5 <- arr[j]
    blt  t1, t5, pt_next
    slli t6, t2, 2      # swap(arr[i], arr[j])
    add  t6, t6, a0
    lw   a3, 0(t6)
    sw   t5, 0(t6)
    sw   a3, 0(t4)
    addi t2, t2, 1      # i++
pt_next:
    addi t3, t3, 1      # j++
    j    pt_loop
pt_done:
    addi a0, t2, -1     # el pivote quedo en i-1
    ret
