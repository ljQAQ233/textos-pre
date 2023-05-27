ENTRY(KernelInit)

. = 0x100000;

SECTIONS {
    .text BLOCK(4K) : ALIGN(4K) {
        *(.text)
    }
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }
    .bss BLOCK(4K) : ALIGN(4K) {
        *(.bss)
    }
}

