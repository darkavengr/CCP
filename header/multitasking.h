void disablemultitasking(void);
void enablemultitasking(void);
void init_multitasking(void);
PROCESS *find_next_process_to_switch_to(void);
size_t is_multitasking_enabled(void);
size_t is_current_process_ready_to_switch(void);


