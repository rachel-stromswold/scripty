#ifndef DTG_VALUES_H
#define DTG_VALUES_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "errors.h"
#include "utils.h"

#ifdef __cplusplus 
extern "C" {
#endif

#define LO_NIB		0x0Fu
#define HI_NIB		0xF0u
#define TOP_BIT		0x80u
#define BIT_UNRES_NAME	0x40u //this indicates that a name was found but that it couldn't be resolved into a literal value. This let's us try to perform a substitution later with names in the current context

//TODO: decide whether using the same value to represent undefined values and errors is a bad idea
#define VT_UNDEF	0
#define VT_ERROR	0
#define VT_CHAR		1
#define VT_BOOL		2
#define VT_INT		3
#define VT_FLOAT	4
#define VT_STRING	5
#define VT_ARRAY	6
#define VT_FUNC		7
#define VT_REF		8
#define VT_OPREF	9
#define N_VALTYPES	11

//VT_CONST and VT_VALUE are boolean flags val & VT_CONST != 0 indicates that the value cannot accept assignments.
/*#define VT_CONST	0x80
#define VT_VALUE	0x40*/

//const size_t VAL_SIZES[N_VALTYPES] = {sizeof(sc_error), 1, 1, sizeof(int), sizeof(double), sizeof(String), sizeof(Array), sizeof(Func)};

#define SZ_BOOL	1
#define SZ_CHAR	1
#define SZ_INT	8
#define SZ_UINT	8
#define SZ_FLT	8

#define DEFAULT_STRING_SIZE	32
#define DEF_ARR_N		4
#define STRING_GROW_SIZE 	4

//these constants define how many bytes to allocate for different data types
#define DEF_ELEMENT_CHARS	8//defines how many characters are allocated for an element of unknown type
#define BOOL_STRING_GROW	6
#define INT_STRING_GROW		8

//constants used for hash tables
#define DEF_TABLE_SIZE		4
#define GROW_THRESH		0.6//specify the threshold of n_els/capacity above which growth is required
#define MAX_KEY_SIZE		32
#define FNV_OFFSET_BIAS		0x53c27916
#define FNV_PRIME		0x811c9dc5 

//constants used for program stacks
#define DEF_STACK_SIZE		4
#define ST_GROW_THRESH		0.6//specify the threshold of n_els/capacity above which growth is required

extern int errno;

// ================================== STRUCT DEFINITIONS ==================================

//VT_VALUE is a special element type which is only valid for arrays if an array has an element_type of VT_VALUE then the buf pointer is an array of value structs with dynamically inferred type
//typedef enum {VT_ERROR, VT_CHAR, VT_BOOL, VT_INT, VT_FLOAT, VT_STRING, VT_ARRAY, VT_FUNC, VT_VALUE} Valtype_e;
typedef char Valtype_e;

//typedef enum {INS_ASSN, INS_MATH, INS_BRANCH, VT_INT, VT_FLOAT, VT_STRING, VT_ARRAY, VT_FUNC, VT_VALUE} Instruction;

/**
 * The PrimArray struct is an alternative implementation of the Array which is theoretically faster than Array.
 * el_size: the size of each element
 * buf_size: the size of the allocated buffer in the number of elements. The total number of bytes allocated for the buffer is el_size*buf_size
 * size: the size of the array that has been written to with valid contents
 */
typedef struct PrimArray {
    Valtype_e element_type;
    size_t buf_size;
    size_t size;
    void* buf;
    size_t el_size;
} PrimArray;

/**
 * The String struct is similar to Array, but specifically for holding a buffer of chars.
 * buf_size: the size of the allocated buffer in the number of elements. The total number of bytes allocated for the buffer is el_size*buf_size
 * size: the size of the array that has been written to with valid contents
 */
typedef struct String {
    size_t buf_size;
    size_t size;
    char* buf;
} String;

union Primtype {
    int i;
    double f;
    String* str;
    void* ptr;
};

/**
 * The value struct specifies values used internally by the language. Contents are stored in the buffer val which has a size val_size and read operations perform casts to extract the proper result. 
 * type: the type of data stored in this value used for assignments and evaluations
 * val_size: the size in bytes of the value pointed to by val. For all types apart from string this is known by the type.
 * val: the raw byte representation of the value
 */
typedef struct value {
    Valtype_e type;
    union Primtype val;
} value;

/**
 * The Array struct holds dynamically sized arrays of values.
 * buf_size: the size of the allocated buffer in the number of elements. The total number of bytes allocated for the buffer is sizeof(value)*buf_size
 * size: the size of the array that has been written to with valid contents
 */
typedef struct Array {
    Valtype_e element_type;
    size_t el_size;
    size_t buf_size;
    size_t size;
    value* buf;
} Array;

/**
 * The Reference struct stores "shared pointers" to values which keep track of the number of users through a refcount. Most users shouldn't ever call _free_Reference() which frees the memory used by the reference and may lead to segfaults. Instead, the release() function should be called which will decrease the refcount by one. Once the refcount is zero, the garbage collector will call _free_Reference() for you.
 * refcount: The number of references to the value currently being used. Changing this is almost certainly a very bad idea.
 * ptr: pointer to the value struct used
 */
typedef struct Reference {
    _uint refcount;
    value* ptr;
} Reference;

/**
 * This is a helper struct which is used by MemoryManager's hash table for value names
 */
typedef struct HashedItem {
    char* key;
    value val;
    //size_t ind;
} HashedItem;

typedef struct s_HashTable {
    size_t table_size;
    size_t n_els;
    HashedItem* table;
} HashTable;

/**
 * The stack is a FILO data structure that supports the operations push() and pop() operations.
 * Note that the bottom of the stack is HIGHER in memory than the top and push instructions append values to lower memory.
 */
typedef struct s_Stack {
    size_t cap;//size of the stack (not in bytes but number of value entries)
    value* bottom;//pointer to the bottom of the stack
    value* top;//pointer to the top of the stack
    value* block;//the block of memory allocated for the stack
} Stack;

/**
 * The stack is a FILO data structure that supports the operations push() and pop() operations.
 */
typedef struct s_NamedStack {
    size_t cap;//size of the stack (not in bytes but number of value entries)
    HashedItem* bottom;
    HashedItem* top;
    HashedItem* block;
} NamedStack;

/**
 * The context struct describes the state of the program at a given point in time. Its primary purpose is to hold the program stack and translate "heap" variable names into usable value structs.
 */
typedef struct context {
    NamedStack callstack;
    HashTable global;
} context;

// ================================== GENERAL VALUE FUNCTIONS ==================================

/**
 * Frees the value pointed to by p_val. If p_val is an array like object, then free_val will properly free the stored array as well. In most cases, end users shouldn't need to call lower level functions like free_Array().
 */
void free_value(value* p_val);

/**
 * Returns the length of the value p_val. Arrays and strings return the number of elements while primitive types all return 1. It is guaranteed that for any int k where 0 <= k < len(p_val), a(p_val, k) will be valid.
 */
size_t len(value p_val);

/**
 * Helper function that returns the string where contents for the string type value p_val are stored. This function is only valid for strings, for arrays use _get_buf(). In the event that this function is called for a non string type NULL is returned.
 * WARNING: calls to grow() or append() may invalidate the buffer
 */
char* _get_char_buf(value p_val);

/**
 *  This is a utility function which fetches the value a stored inside the array like object p_val. For strings this returns a value with the appropriate character stored within. For primitive types only i=0 is valid and returns a copy with information identical to p_val.
 *  NOTE: when applicable, deep copies are performed. The caller is responsible for ensuring that memory allocated by this function is freed (see free_value()).
 */
char a(value p_val, int i, sc_error* err);

/**
 * Perform a deep copy of the value p_val. This function is guaranteed not to error.
 */
value v_deep_copy(value p_val, sc_error* err);

/**
 * Returns the length of the stringified version of a value
 */
size_t get_format_string_size(value p_val, size_t precision);

/**
 * Returns the length of the stringified version of a value
 */
value read_value_string(char* str, Valtype_e hint, sc_error* err);

/**
 * Reads an input stream str into a value of type hint. If hint is set to 0 (VT_ERROR), the type is inferred based on inputs.
 * NOTE: boolean values are by default interpreted as true. To set a boolean to false str must either be '0' or 'false'.
 * WARNING: str is modified in place and an address returned. You should duplicate the string before calling this function if you need to use the original string.
 */
Valtype_e read_valtype(char* str, sc_error* err);

/**
 * Helper function which reads a string of the form "<type> <name>", "<type> <name> = <val>" or "<name> = <val>" into a hashed item which is used as a name value pair.
 */
HashedItem _read_valtup(char* str, sc_error* err);

/**
 * Frees the memory allocated for usage by h. This assumes a call to _read_valtup has been used or the key and value have both been allocated using DTG_malloc().
 */
void free_HashedItem(HashedItem* h);

// ================================== ARRAYS ==================================

/**
 * Initializes an array of n elements with size el_size or stores a result to the error err.
 * Note: on an error this function may return NULL
 */
Array* _make_Array(size_t el_size, size_t n, sc_error* err);

/**
 * Frees the array pointed to by arr
 */
void free_Array(Array* arr);

/**
 * Grows the array arr to accomodate n additional entries of size el_size
 */
void _grow_a(Array* arr, size_t n, sc_error* err);

/**
 * Grows the array arr to accomodate n additional entries of size el_size
 */
void _grow_pa(PrimArray* arr, size_t n, sc_error* err);

/**
 * Resizes the array arr to hold exactly n entries of size el_size. If n is less than the current size of the array, then elements at the end are discarded.
 */
void _resize(Array* arr, size_t n, sc_error* err);

/**
 * Returns a deep copy of the array pointed to by arr
 */
Array _copy_a(Array arr, sc_error* err);

/**
 * Returns a new array with elements ranging from start_ind to end_ind of the original array. A shallow copy is made, and the contents of the returned array only have a lifespan matching arr. Call _copy_a() before this function if you need a deep copy.
 * Note negative values "wrap around", so _slice(arr, -2, -1, NULL) would return the second to last element in the array. Indices greater than the size of the array are fixed to be equal to the size of the array.
 */
Array _slice_a(Array arr, long int start_ind, long int end_ind, sc_error* err);

/**
 * This function is identical to _slice a, but it operates on PrimArrays.
 */
//PrimArray _slice_pa(PrimArray arr, long int start_ind, long int end_ind, sc_error* err);

/**
 * Appends the array of values of length n specified by new vals to the end of the Array pointed to by arr. A deep copy of elements is performed.
 */
void _extend_a(Array* arr, value* new_vals, size_t n, sc_error* err);

// ================================== ARRAY VALUES ==================================

/**
 * Creates a new array value with contents identical to those stored in p_val (a deep copy is performed.
 * NOTE: A deep copy of p_val is made. Thus, no guarantees as to the lifespan of the memory pointed to by p_val are required.
 */
value v_make_array(const value* p_val, size_t n_vals, sc_error* err);

/**
 * Creates a new empty array value with memory allocated to hold n members of type p_eltyp.
 */
value v_make_array_n(size_t p_n, value tmplt, sc_error* err);

/**
 * Returns: A deep copy of the value array p_val.
 */
value v_copy_array(value p_val, sc_error* err);

// ================================== STRINGS ==================================

/**
 * Creates a new String with contents indicated by p_str. A deep copy of the string is performed and memory should be deallocated by a matching call to free_String().
 */
String make_String(const char* p_str, sc_error* err);

/**
 * Creates an uninitialized empty string with the capacity to hold n bytes without any additional calls to grow
 */
String make_String_n(size_t n, sc_error* err);

/**
 * Frees the memory used by str. after a call to free_String the String str still has not been allocated, but it is safe to call DTG_free(str) if the string itself was malloced.
 */
void free_String(String* str);

/**
 * Grows the string value val to accomodate n additional bytes.
 */
void _grow_s(String* val, size_t n, sc_error* err);

/**
 * Reallocates the buffer used by val so that it holds exactly n bytes.
 */
void _resize_string(String* val, size_t n, sc_error* err);

/**
 * Appends the string p_str to the string value val, resizing the buffer to accomodate results if necessary.
 */
void _append_string(String* val, const char* p_str, sc_error* err);

/**
 * Appends the string contained in the value o_val to val, resizing the buffer to accomodate results if necessary.
 */
void _append_string_s(String* val, String o_val, sc_error* err);

// ================================== STRING VALUES ==================================

/**
 * Helper function that returns the character buffer used by string values
 * Note: ideally this function should only be used to fetch read only data. Call resize or grow functions to adjust the allocated size of the array
 */
char* get_string(value* p_val, sc_error* err);

/**
 * Creates a new string value with val identical to p_val.
 * NOTE: a deep copy of p_val so no guarantees as to the lifespan of the memory pointed to by p_val are required
 */
value v_make_string(const char* p_val, sc_error* err);

/**
 * Creates a new empty string value with memory allocated to hold n bytes.
 * NOTE: a deep copy of p_val so no guarantees as to the lifespan of the memory pointed to by p_val are required
 */
value v_make_string_n(size_t p_n, sc_error* err);

/**
 * Fetches the string stored in p_val and performs casts if necessary.
 * Returns: the number of characters written to the buffer p_str.
 */
int v_fetch_string(value p_val, char* p_str, size_t n, sc_error* err);

// ================================== BOOLS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_bool(int p_val, sc_error* err);

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
int v_fetch_bool(value p_val, sc_error* err);

// ================================== CHARS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_char(char p_val, sc_error* err);

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
int v_fetch_char(value p_val, sc_error* err);

// ================================== INTS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_int(int p_val, sc_error* err);

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
int v_fetch_int(value p_val, sc_error* err);

// ================================== FLOATS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_float(double p_val, sc_error* err);

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
double v_fetch_float(value p_val, sc_error* err);

//char* _get_enclosed(char* str, const char* open, const char* close);

// ================================== REFERENCES ==================================

/**
 * Frees the Reference struct pointed to by r. You should call release() instead if you want refcounts to change properly
 */
void _free_Reference(Reference* r);

/**
 * Releases the Reference pointed to by r. This function should be called by end users when they no longer need an object. When the refcount reaches zero the garbage collector will call _free_Reference().
 */
void release(Reference* r);

// ================================== MEMORY MANAGEMENT AND HASHING ==================================

/**
 * Uses FNV-1a with the bias and prime defined previously to compute the hash of the specified key. This function is used internally by lookup(), insert_deep() and insert(). This hash is not cryptographically secure.
 */
_uint32 hash(const char* key);

/**
 * This function is identical to hash(), but it also stores the length of the key string into keylen.
 */
_uint32 hash_l(const char* key, size_t* keylen);

/**
 * Creates a new HashTable and sets appropriate values. You can remove it when you are done using free_HashTable.
 */
HashTable make_HashTable(sc_error* err);

/**
 * Deallocates the HashTable pointed to by h.
 */
void free_HashTable(HashTable* h);

/**
 * Look through the hash table for an entry that matches the supplied key
 * param h: the hash table to look through
 * param key: the key of the desired entry
 * returns: a pointer to the associated value. The user should not attempt to free this value.
 */
HashedItem* lookup(HashTable* h, const char* key);

/**
 * Insert a new item into the hash table with the specified key and value. Note that a shallow copy of the contents of val are performed. In order to produce a deep copy use insert_deep instead.
 * param h: the hash table to look through
 * param key: the key of the entry to create
 * param val: the value to be inserted
 */
void insert(HashTable* h, const char* key, value val, sc_error* err);

/**
 * Insert a new item into the hash table with the specified key and value. Note that a deep copy of the contents of val are performed if the value is of a non primitive type. This may be slower than using insert() so using a call to that function may be prudent.
 * param h: the hash table to look through
 * param key: the key of the entry to create
 * param val: the value to be inserted
 */
void insert_deep(HashTable* h, const char* key, value val, sc_error* err);

// ================================== STACK ==================================

/**
 * Creates a new (empty) stack. The returned Stack has a pointer to the invalid index -1 and a call to push() must be made before any calls to pop()
 */
Stack make_Stack(sc_error* err);

/**
 * Frees the stack pointed to by st and any memory allocated for its contained members.
 */
void free_Stack(Stack* st);

/**
 * Pushes the value p_val onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push(Stack* st, value p_val, sc_error* err);

/**
 * Pushes an uninitialized value of type t onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push_uninit(Stack* st, Valtype_e t, sc_error* err);

/**
 * Pops the last value off of the stack and returns the result. After a call to pop a shallow copy is made and the caller is responsible for freeing any memory which may have been allocated.
 */
value pop(Stack* st, sc_error* err);

/**
 * Returns 1 if the stack pointed to by st is empty or 0 otherwise. Upon an error or invalid value, -1 is returned.
 */
int is_empty(Stack* st);

/**
 * Get the size of the NamedStack pointed to by st
 */
size_t get_size(Stack st);

// ================================== NAMED STACK ==================================

/**
 * Creates a new (empty) stack. The returned Stack has a pointer to the invalid index -1 and a call to push() must be made before any calls to pop()
 */
NamedStack make_NamedStack(sc_error* err);

/**
 * Frees the stack pointed to by st and any memory allocated for its contained members.
 */
void free_NamedStack(NamedStack* st);

/**
 * Pushes the value p_val onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push_n(NamedStack* st, char* name, value v, sc_error* err);

/**
 * Pops the last value off of the stack and returns the result. After a call to pop a shallow copy is made and the caller is responsible for freeing any memory which may have been allocated.
 */
HashedItem pop_n(NamedStack* st, sc_error* err);

/**
 * Returns 1 if the stack pointed to by st is empty or 0 otherwise. Upon an error or invalid value, -1 is returned.
 */
int is_empty_n(NamedStack* st);

/**
 * Access the key of the stack element at index ind. If ind is out of bounds, NULL is returned.
 */
char* read_key(NamedStack* st, size_t ind);

/**
 * Access the value of the stack element at index ind. If ind is out of bounds, an invalid value is returned.
 */
value read_value(NamedStack* st, size_t ind);

/**
 * Get the size of the NamedStack pointed to by st
 */
size_t get_size_n(NamedStack st);

// =============================== CONTEXTS ===============================

/**
 * Search for the value with the name name
 */
context make_context(sc_error* err);

/**
 * Free the memory allocated for the context pointed to by c.
 */
void free_context(context* c);

/**
 * Search for the value with the name name. The hashtable is searched first and if the value is found there, then -1 is returned. In the event that the entry is not found in the table, the stack is searched. If the entry with the matching name is found, then the index (relative to th bottom of the stack) is stored into off. If the entry is not found, -2 is returned. If val is not NULL, then the resulting value will be stored there or NULL if no matching value was found
 */
int search_val(context* c, const char* name, HashedItem** val);

/**
 * Add a value to the context c with the name name
 */
value* add_val(context* c, const char* name, sc_error* err);

#ifdef __cplusplus 
}
#endif

#endif
