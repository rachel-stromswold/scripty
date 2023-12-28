#ifndef DTG_OPERATIONS_H
#define DTG_OPERATIONS_H

#include "values.h"

#ifdef __cplusplus 
extern "C" {
#endif

#define DEF_ARG_CAP	16

#define N_WORD_BYTES	16

#define N_INS_TOKS	10
#define DEF_NUM_INS	15

#define IND_LIST_SIZE	16
#define FLAG_NORMAL	0
#define FLAG_OPER	1
#define FLAG_COMP	2
#define FLAG_AND	3

#define INS_NOP		0x00u
#define INS_OP_EVAL	0x01u
#define INS_FN_EVAL	0x02u
#define INS_JUMP	0x03u
#define INS_JUMP_CND	0x04u
#define INS_PUSH	0x05u
#define INS_POP		0x06u
#define INS_MOV		0x07u
#define INS_PTR_DRF	0x08u
#define INS_GET_SIZE	0x09u
#define INS_IND_READ	0x0Au
#define INS_IND_WRITE	0x0Bu
#define INS_FL_OPEN	0x0Cu
#define INS_FL_CLOSE	0x0Du
#define INS_FL_READ	0x0Eu
#define INS_FL_WRITE	0x0Fu
#define INS_MAKE_PTR	0x10u
#define INS_MAKE_ARR	0x11u
#define INS_MAKE_STR	0x12u
#define INS_MAKE_VAL	0x13u
#define INS_EXT		0x14u
#define INS_RETURN	0x15u

//these are special temporary instructions which
#define BLOCK_WHILE		0
#define BLOCK_BRANCH		1
#define BLOCK_SUB_BRANCH	2
#define BLOCK_ELSE		3

//define the high and low masks that are compared against
#define INS_HL		0x30u
#define INS_HH		0xC0u
//define high masks which specify which bank instructions are read from
#define INS_HL_R	0x00u //register
#define INS_HL_S	0x10u //stack
#define INS_HL_G	0x20u //global
#define INS_HL_C	0x30u //constant (instruction buffer)
#define INS_HH_R	0x00u
#define INS_HH_S	0x40u
#define INS_HH_G	0x80u
#define INS_HH_C	0xC0u

/*#define INS_NOP		0//params 0
#define INS_EVAL	1//params 1: interprets the next instruction as a pointer to an Operation struct to evaluate, the result is pushed onto the stack
#define INS_ENDBLK	2//The instructions INS_WHILE and INS_BRANCH 
#define INS_WHILE	3//params 2: Interprets the next instruction as a pointer to an Operation struct which is evaluated. The next parameter specifies the number of instructions contained in the while block. If the result of this evaluation is a boolean true, then we proceed with execution of the main loop. Otherwise, we jump to the first instruction after the end of the loop. The instruction after that is interpreted as a pointer to the number of instructions contained within the loop.
#define INS_JUMP	4//params 1: The next instruction is interpreted as an index in the instruction pipeline which we jump to unconditionally.
#define INS_JNCOND	5//params 2: Interprets the next instruction as a pointer to an Operation struct which is evaluated. The next instruction is interpreted as an index in the instruction pipeline. If the result of the evaluation is a boolean false, then we jump to to the specified instruction index. Otherwise, we continue execution with the first instruction after the index.
#define INS_BRANCH	6//params 2: Interprets the next instruction as a pointer to an Operation struct which is evaluated. If the result of this evaluation is a boolean true, then we proceed with execution of the code inside the branch. The next instruction after the Operation pointer is interpreted as the number of instructions contained within the true execution branch. If the operation is evaluated as false, then the instruction pointer is advanced by this indicated number. Using this it is possible to chain together multiple if statements (i.e. an if() {} else if() {} pair)
#define INS_PUSH	7//params 1: The next instruction is interpreted as an instruction indicating the register number to read from. The value pointed to by the register is dereferenced and pushed onto the stack.
#define INS_POP		8//params 1: The next instruction is interpreted as an instruction indicating the register number to write to. The top of the stack is popped off and stored into this register.
#define INS_MOV_RGL	7+//params 2: The next instruction is interpreted as an instruction indicating the register number to write to. The value at this stack index is fetched (but not removed) and a pointer to this value is pushed onto the stack.
#define INS_MOV_RST	135//params 1: The next instruction is interpreted as an unsigned integer which represents the offset from the bottom of the stack. The value at this stack index is fetched (but not removed) and a pointer to this value is pushed onto the stack.
#define INS_POP_ST	136//params 1: The next instruction is interpreted as an unsigned integer which represents the offset from the bottom of the stack. The top value on the stack is popped off and stored in this value.
#define INS_PUSH_A	9//params 2: The next instruction is interpreted as a pointer to a value struct with array type, call this a. The second instruction is interpreted as an integer index, call this i. The array value at index i is then pushed onto the stack.
#define INS_POP_A	10//params 2: The next instruction is interpreted as a pointer to a value struct with array type, call this a. The second instruction is interpreted as an integer index, call this i. The top value on the stack is popped off and stored in this a[i].
#define INS_PUSH_A_ST	137//params 2: The next instruction is interpreted as an unsigned integer which represents the offset from the bottom of the stack, call this a. The second instruction is interpreted as an integer index, call this i. The value at this stack index is fetched (but not removed) and a pointer to the value at a[i] is pushed onto the stack.
#define INS_POP_A_ST	138//params 2: The next instruction is interpreted as an unsigned integer which represents the offset from the bottom of the stack, call this a. The second instruction is interpreted as an integer index, call this i. The top value on the stack is popped off and stored in a[i].
#define INS_FUNC_EVAL	11//params variable: Interprets the next instruction as a pointer to a function struct. The function is fetched and execution is handed over to the other function with arguments popped off of the stack. After completion, the returned values are pushed onto the stack.
#define INS_FUNC_EVAL_ST	139//params 1: Interprets the next instruction as a stack index. The function is fetched from the stack at the index and execution is handed over to the other function with arguments popped off of the stack. After completion, the returned values are pushed onto the stack.
#define INS_RETURN	12//params variable: Interprets the next N instructions as stack indices to push onto the return stack and then returns execution to the calling function. N is provided in the definition of the function, thus if there are multiple return points in a function then ALL must have the same return signature.
#define INS_ASSN_OP	13//params 2: Interprets the next instruction as a pointer to a value struct, call this val. The following instruction is interpreted as a pointer to an Operation struct. The operation is evaluated and the result is stored into val.
#define INS_ASSN_CNST	14//params 2: Interprets the next instruction as a pointer to a value struct, call this lval. The next instruction is also interpreted as a pointer to a value struct which we call rval. Memory in lval is freed as necessary and a deep copy is performed, after which point both lval and rval have the contents rval had prior to the instruction (i.e. lval = rval).
#define INS_MOV_CNST	15//params 2: Interprets the next instruction as a pointer to a value struct, call this lval. The next instruction is also interpreted as a pointer to a value struct which we call rval. This is similar to INS_ASSN_CNST, except the value pointed to by rval is invalidated after operation. Instead of a deep copy, a shallow copy is performed which may be faster.
#define INS_ASSN_VT	16//params 2: This is actually a collection of functions which behave in a similar manner.The first instruction after INS_ASSN_VT is interpreted as a value pointer, just as is the case in INS_ASSN_CNST. However, unlike that function, the following instruction is interpreted as a byte for byte representation of the contents of a value with type INS - INS_ASSN_VT where INS is the subinstruction which was called.
#define INS_MOV_VT	17//params 2: This collection of instructions is identical to INS_ASSN_VT except shallow copies are made which move the contents into the lval.
#define INS_BR_ELSE	18//params 0: This is equivalent to INS_NOP but is used as a temporary flag before final compilation that represents an else block
#define INS_BRANCH_SUB	19//params 2: This is a temporary instruction that is replaced with INS_BRANCH after the final compiler passthrough. We use this temporary instruction, because at the first pass we don't know how many instructions are contained within the proceeding conditional statements. As such, the second parameter is left uninitialized and invalid until the second passthrough is complete.*/

typedef enum {NOP = 0, OP_ASSN, OP_EQ, OP_ADD, OP_SUB, OP_MULT, OP_DIV, OP_NOT, OP_OR, OP_AND, OP_GRT, OP_LST, OP_GEQ, OP_LEQ, N_OPTYPES} Optype_e;

/**
 * Operations form a binary tree with each node containing an operator to apply to both children and each leaf consisting of a single floating point value
 */
struct Operation {
    Optype_e op;
    value val;
    struct Operation* child_l;
    struct Operation* child_r;
};

/**
 * This is a helper struct used by function to describe individual instructions (pseudo bytecodes interpreted by the interpreter).
 */
union Instruction {
    size_t i;
    void* ptr;
};

typedef struct s_instruction_buffer {
    size_t cap;
    size_t n_insts;
    union Instruction* buf;
} instruction_buffer;

/**
 * The function struct holds functional types
 * el_size: the size of each element
 * buf_size: the size of the allocated buffer in the number of elements. The total number of bytes allocated for the buffer is el_size*buf_size
 * size: the size of the array that has been written to with valid contents
 */
typedef struct s_function {
    size_t n_args;
    size_t n_rets;
    size_t n_consts;
    Valtype_e* argument_types;
    Valtype_e* return_types;
    value* consts;
    instruction_buffer buf;
} function;

// ============================ MATHEMATICAL OPERATIONS ============================

/**
 *  Creates a new value object which holds the result of the addition operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  string a, (any type b): Converts b into a string representation and appends the result to the end of a.
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a+b.
 *  int a, int b: Returns a+b
 *  array a, (any type b): This is an invalid operation unless the type of b matches the type stored in a, in which case a deep copy of b is made and appended to the end of a.
 *  bool a, bool b: invalid
 */
value op_add(value a, value b, sc_error* err);

/**
 *  Creates a new value object which holds the result of the subtraction operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a-b.
 *  int a, int b: Returns a-b
 *  array a, (any type b): invalid
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_sub(value a, value b, sc_error* err);

/**
 *  Creates a new value object which holds the result of the multiply operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a*b.
 *  int a, int b: Returns a*b
 *  array a, (float or int b): Performs a scalar multiply to each element of a so that for each element e of a the new array has the same value as a call to mult(e, a) would. Note that a deep copy of the data is performed, so changes to a may be made safely and the result must be freed.
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_mult(value a, value b, sc_error* err);

/**
 *  Creates a new value object which holds the result of the divide operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a/b.
 *  int a, int b: Returns the greatest integer less than a/b (i.e. floor(a/b)).
 *  array a, (float or int b): Performs a scalar multiply to each element of a so that for each element e of a the new array has the same value as a call to mult(e, a) would.
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_div(value a, value b, sc_error* err);

/**
 *  Creates a new boolean value object which is set to true if a and b are equal.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays a, b: returns true if op_eq evaluates true for each pair of elements from a and b
 *  strings a, b: returns true if each character in strings
 *  bool a, bool b: equivalent to (a and b) or (~a and ~b)
 */
value op_eq(value a, value b, sc_error* err);

/**
 *  Creates a new boolean value object which is set to true if a is greater than b.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays invalid
 *  strings a, b: returns true if a comes before b alphabetically
 *  bool a, bool b: invalid
 */
value op_grt(value a, value b, sc_error* err);

/**
 *  Creates a new boolean value object which is set to true if a is greater than or equal to b.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays invalid
 *  strings a, b: returns true if a comes before b alphabetically
 *  bool a, bool b: invalid
 *  NOTE: there is no low level implementation of < or <=, these are evaluated by reversing parameters.
 */
value op_geq(value a, value b, sc_error* err);

// ============================ OPERATION TREES ============================

/**
  * Recursively evaluates the operation tree with the root specified by o. All values are treated as floats during calculation. For integer arithmetic use evali().
  * Returns: the value of the operation tree. Note that boolean operations consider 0.0 false and all other values true.
  */
value eval(struct Operation* o, Stack* st, sc_error* err);

/**
  * Helper function which parses a string expression into a tree of operations. The optree can then be evaluated using a call to eval() or evali()
  */
struct Operation* gen_optree(char* str, NamedStack* st, sc_error* err);

/**
 * Frees the operation pointed to by op and all of its children
 */
void free_Operation(struct Operation* op);

// ============================ Instruction Buffer ============================

/**
 * make a new empty instruction_buffer struct
 */
instruction_buffer make_instruction_buffer(sc_error* err);

/**
 * Append the instruction inst to the end of buf and store potential errors in err
 */
void append_Instruction(instruction_buffer* buf, union Instruction inst, sc_error* err);

/**
 * Append n_in instructions from the array inst to the end of buf and store potential errors in err
 */
void append_Instructions(instruction_buffer* buf, size_t n_in, union Instruction* inst, sc_error* err);

/**
 * Deallocate memory used by the instruction_buffer buf
 */
void free_instruction_buffer(instruction_buffer* buf);

// ============================ FUNCTIONS ============================

/**
 * Helper function to lookup the stack index associated with a given stack variable. If the given name is not found -1 is returned.
 */
int get_stack_ind(size_t stack_size, char** name_stack, char* name);

/**
 *  A helper function to parse the string str (of the form <type> <name>) into a type and value string.
 *  param str: string to parse, including the type
 *  param type: the type to cast the resulting value to
 *  param off: the offset specifying the end of the type specifier string in str
 *  param n_st: named stack to push the new value to
 *  param i_buf: instruction buffer to write to
 *  returns: the number of characters from the name which were read, see the specifications for read_dtg_word
 */
size_t __read_declaration(char* str, Valtype_e type, size_t off, context* c, instruction_buffer* i_buf, sc_error* err);

/**
 * Interprets the string str into an executable function. The number of arguments and return values are interpreted.
 */
function make_function(context* con, char* str, sc_error* err);

/**
 * Free memory used by the function pointed to by f.
 */
void free_function(function* f);

/**
 * This is a helper function which reads the specified token and pushes it onto the named stack st.
 * Returns: a duplicate (shallow copy) of the HashedItem struct that was just pushed onto the stack. This happens even if an error was returned so that the caller may perform additional error resolution.
 */
HashedItem _push_valtup(NamedStack* st, char* token, sc_error* err);

/**
 * Helper function which parses an rval string into a sequence of instructions. This is done by recursively looking up values from the provided context and replacing with optrees or functions to evaluate where appropriate.
 */
int _parse_rval(context* c, char* str, int force_single, instruction_buffer* buf, sc_error* err);

/**
 * This is a helper function which reads the specified token and pushes it onto the named stack st.
 */
//size_t _add_instruction(union Instruction** i_list, , sc_error* err);

/**
 * Executes the function f using the context c to infer the stack and global variables. The context is altered by this proceedure as f pops arguments off of the stack (including the caller) and pushes returned values onto the stack.
 */
void execute_function(context* c, function* f, sc_error* err);

/**
 * Lookup the function named func_name and an array of arguments args and n_args
 */
int call_func_by_name(context* c, const char* func_name, size_t n_args, char** args, value** results, sc_error* err);

#ifdef __cplusplus 
}
#endif

#endif //DTG_OPERATIONS_H
