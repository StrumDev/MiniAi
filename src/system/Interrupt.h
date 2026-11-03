#pragma once

// Lets Ctrl+C stop a long job politely instead of killing the program.
void InterruptArm(void);
void InterruptDisarm(void);
int InterruptRequested(void);
