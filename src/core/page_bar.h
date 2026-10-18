#pragma once

// Digit bar test screen: 1234567890 in a bar at the bottom, the selected digit shown large above it.
// Call digitBarInit() once in setup(), after playBootAnimation(); pass every typed char to digitBarHandleKey().
void digitBarInit();

// A digit selects it; '<' or ',' moves to the previous one, '>' or '/' to the next (both wrap around).
void digitBarHandleKey(char c);
