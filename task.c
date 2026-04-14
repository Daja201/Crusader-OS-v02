#include "task.h"
#include <stdint.h>
#include "pmm.h"

#define MAX_TASKS 16
extern volatile uint32_t system_ticks; // Doplň nahoru k includům
// Globální pole procesů a počítadla
task_t tasks[MAX_TASKS];
int current_task = -1;
int num_tasks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}


void task_exit() {
    asm volatile("cli");
    // Zde by v budoucnu mělo být uvolnění paměti a odstranění z fronty
    tasks[current_task].state = TASK_READY; // Pro tebe zatím jen stopne task
    for(;;); 
}

void init_multitasking() {
    tasks[0].pid = 0;
    tasks[0].state = TASK_RUNNING;
    num_tasks = 1;
    current_task = 0;
}

uint32_t schedule_handler(uint32_t esp) {
    outb(0x20, 0x20);
    system_ticks++;
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
    return tasks[current_task].esp;
}
void create_task(void (*entry_point)()) {
    if (num_tasks >= MAX_TASKS) return;
    void* stack_mem = pmm_alloc_block(); 
    if (!stack_mem) return;
    uint32_t *stack = (uint32_t *)((uint32_t)stack_mem + 4096);
    *(--stack) = (uint32_t)task_exit;
    *(--stack) = 0x0202;                 // EFLAGS
    *(--stack) = 0x10;                   // CS
    *(--stack) = (uint32_t)entry_point;  // EIP
    *(--stack) = 0;                      // ErrCode
    *(--stack) = 32;                     // INT 32
    for (int i = 0; i < 8; i++) *(--stack) = 0; 
    for (int i = 0; i < 4; i++) *(--stack) = 0x18; 
    tasks[num_tasks].esp = (uint32_t)stack;
    tasks[num_tasks].pid = num_tasks;
    tasks[num_tasks].state = TASK_READY;
    num_tasks++;
}