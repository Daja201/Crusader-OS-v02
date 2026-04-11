#include "task.h"
#include <stdint.h>

#define MAX_TASKS 16

// Globální pole procesů a počítadla
task_t tasks[MAX_TASKS];
int current_task = -1;
int num_tasks = 0;

void init_multitasking() {
    tasks[0].pid = 0;
    tasks[0].state = TASK_RUNNING;
    num_tasks = 1;
    current_task = 0;
}
// ---------------------------------------------------------
// 1. PLÁNOVAČ (volaný z isr32 v interrupts.s)
// ---------------------------------------------------------
uint32_t schedule_handler(uint32_t esp) {
    // Pokud máme 0 nebo 1 proces, není mezi čím přepínat
    if (num_tasks <= 1) {
        return esp;
    }

    // Pokud už nějaký proces běžel, uložíme mu jeho aktuální zásobník
    if (current_task >= 0) {
        tasks[current_task].esp = esp;
    }

    // Vybereme další proces v pořadí (kruhové střídání)
    current_task++;
    if (current_task >= num_tasks) {
        current_task = 0; // Jsme na konci, jedeme zase od nuly
    }

    // Vrátíme ESP nového procesu (Assembly ho nahraje do procesoru)
    return tasks[current_task].esp;
}

// ---------------------------------------------------------
// 2. VYTVOŘENÍ NOVÉHO PROCESU
// ---------------------------------------------------------
void create_task(void (*entry_point)(), uint32_t *stack_top) {
    if (num_tasks >= MAX_TASKS) return;

    uint32_t *stack = stack_top;

    // A) Co vkládá procesor (čte to iret)
    *(--stack) = 0x0202;                 // EFLAGS
    *(--stack) = 0x10;                   // CS (Kernel Code Segment)
    *(--stack) = (uint32_t)entry_point;  // EIP (Kde má proces začít)

    // B) Co vkládáme my v našem isr32 makru úplně nahoře
    *(--stack) = 0;                      // Error code (dummy)
    *(--stack) = 32;                     // Číslo přerušení (IRQ0)

    // C) pusha (ukládá v tomto přesném pořadí z EAX po EDI)
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // Původní ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    // D) Segmentové registry (poslední, co se pushne, a první, co se popne!)
    *(--stack) = 0x18;                   // DS (Návrat k tvému původnímu 0x18)
    *(--stack) = 0x18;                   // ES
    *(--stack) = 0x18;                   // FS
    *(--stack) = 0x18;                   // GS

    // Uložíme tento zarovnaný zásobník do naší struktury
    tasks[num_tasks].esp = (uint32_t)stack;
    tasks[num_tasks].pid = num_tasks;
    tasks[num_tasks].state = TASK_READY;

    num_tasks++;

}
