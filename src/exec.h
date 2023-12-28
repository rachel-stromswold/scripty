#ifndef EXEC_H
#define EXEC_H

#include "operations.h"

#ifdef __cplusplus 
extern "C" {
#endif

#define N_REGISTERS	4

/**
 * The LiveContext struct describes the state of the program at a given point in time. Its primary purpose is to hold the program stack and translate "heap" variable names into usable Value structs. This differs from the Context struct in that the callstack is not named and only referenced by index.
 */
typedef struct s_LiveContext {
    Stack callstack;
    HashTable global;
} LiveContext;

// ==================================== FUNCTION EXECUTION ====================================

/**
 * Helper function to read the value stored at the stack index ind.
 */
Value _fetch_from_stack(LiveContext* c, size_t ind);

/**
 * Execute the already created function f within the runtime Context c.
 */
int _ex_func(Function f, LiveContext* c, DTG_Error* err);

#ifdef __cplusplus 
}
#endif

#endif
