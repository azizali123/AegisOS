/* ============================================================
   interrupts.S - Kesme Servis Rutinleri (ASM stubs)
   ============================================================ */
.section .text

/* Genel ISR stub — tüm register'ları koru, C işleyicisini çağır */
.global isr_generic
isr_generic:
    pusha
    call c_generic_handler
    popa
    iret

/* IRQ0 — PIT Timer */
.global isr_irq0
isr_irq0:
    pusha
    call c_irq0_handler
    popa
    iret

/* IRQ1 — Klavye */
.global isr_irq1
isr_irq1:
    pusha
    call c_irq1_handler
    popa
    iret
