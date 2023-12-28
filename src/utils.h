#ifndef DTG_UTILS_H
#define DTG_UTILS_H

#include <string.h>
#include "errors.h"

#define CONSERVE_MEM		1
#define DEFAULT_WORD_SIZE	16
#define DEFAULT_STACK_SIZE	8
#define MAX_BLK_RECURSE		255
#define INONE			9223372036854775807

#ifdef __cplusplus 
extern "C" {
#endif

typedef unsigned char _uint8;

/**
 * helper declaration for csv_to_list. Stores types of scoping blocks and their index within a string.
 */
typedef enum {
  BLK_SQUARE = 0,
  BLK_PAREN,
  BLK_CURLY
} blk_type;
typedef struct stype_ind_pair {
    blk_type t;
    size_t i;
} type_ind_pair;

// ==================================== PARSING ====================================

/**
 * Interprets the character d as a digit in the specified base. In the event of an invalid character, -1 is returned.
 */
int _read_digit(char d, _uint base);

/**
 * Returns the portion of the string str between the first match from the set of open and close characters provided. Open and close can be strings containing multiple characters in wich case the enclosed contents will follow the first match for open and the matching close character will be used. No heap memory is allocated by this function.
 * NOTE: For multiple matching open and close characters, open and close MUST be of equal length. Furthermore, the match is based on the index within the strings open and closed. For instance a call to _get_enclosed(str, "({", "})") where str="{foo}" will result in an error (NULL is returned). Instead, the user should call _get_enclosed(str, "({", ")}") so that the curly brace is the second character in both the open and close strings.
 * NOTE: this function preserves "scope". That is, both the open and close characters must be in the same scope. For instance _get_enclosed(str, "(", ")") where str="((foo)bar)" will return a string with contents "(foo)bar" instead of "(foo".
 * WARNING: str is modified in place and an address returned. You should duplicate the string before calling this function if you need to use the original string.
 * Returns: the enclosed contents as described above or NULL if an error is encountered. If no matching open character is encountered then the string is returned unaltered.
 */
char* _get_enclosed(char* str, const char* open, const char* close);

/**
 * This function is identical to _get_enclosed but accepts one additional parameter between str and open, saveptr. If saveptr is not NULL then it is set to the first character after the close parenthesis was found. Additionally, the matching start and end characters are replaced with 0 terminators. The pointer may be set to NULL if an error was encountered or the close brace was the last character in the string.
 */
char* _get_enclosed_r(char* str, char** saveptr, const char* open, const char* close);

/**
 * Get the first instance of the token tok after index i in the string str which is at the same nest level. i.e _search_block("a(b[1,2],c),3", ",", 0) returns 11 (the index of the ',' before the 3)
 * str: the string to search through
 * tok: the token to look for
 * i: the index to start the search
 * returns: the index of the first occurence of the string or i-1 if the token was not found (note the use of unsigned size_t causes underflows so that this function will break if trying to parse a string with more than ~1.8x10^19 characters (so you probably don't have to worry lol.
 */
size_t _search_block(const char* str, const char* tok, size_t i);

/**
 * Convert a string separated by the character sep into a list of strings. For instance, if str == "arg1, arg2" and sep == ',' then the output will be a list of the form ["arg1", "arg2"]. If no sep character is found then the list will have one element which contains the whole string.
 * param str: string to parse into a list
 * param sep: separating character to use.
 * param listlen: location to save the length of the returned list to. Note that this pointer MUST be valid. It is not acceptable to call this function with listlen = NULL. (The returned list is not null terminated so this behavior ensures that callers are appropriately able to identify the length of the returned string.)
 * returns: list of strings separated by commas. This should be freed with a call to DTG_free().
 * NOTE: The input string str is modified and the returned value uses memory allocated for str. Thus, the caller must ensure that str has a lifetime at least as long as the used string. Users should not try to call free() on any of the strings in the list, only the list itself.
 */
char** csv_to_list(char* str, char sep, size_t* listlen, sc_error* err);

/**
 * This function modifies the string in place removing leading and trailing whitespace.
 */
char* _trim_whitespace(char* str);

/**
 * Compares the next (non whitespace) word in the string str with the token tok. This word is terminated by either a whitespace character or a null terminator.
 * If the comparison evaluates to an equivalence a positive integer representing the number of needed to skip to reach the END of the matched token. Otherwise -1 is returned. In the event of an error, -2 is returned.
 */
int cmpnwrd(const char* str, const char* tok);

/**
 * Reads the next whole word (sequence of non whitespace characters) from string str starting at offset off. Up to max_n bytes from this word are read into the string sto (sto is guaranteed to be null terminated). Returns an index (relative to str+off) to the first character AFTER the word that was just read.
 */
size_t read_dtg_word(const char* str, size_t off, char* sto, size_t max_n);

/**
 * Reads the next whole word (sequence of non whitespace characters) from string str starting at offset off. This is performed "in place" modifying the string str. The resulting string is saved to *sto if sto is not NULL. The caller must ensure that the lifespan of sto does not exceed that of str.
 */
size_t read_word_i(char* str, size_t off, char** sto);

/**
 * Creates a new string with memory allocated through DTG_malloc. The contents of s are copied into this memory. If CONSERVE_MEM is defined, then the memory is resized to exactly hold the specified contents. In the event of an error, NULL is returned.
 */
char* DTG_strdup(const char* s, sc_error* err);

// ================================== LOOKUP TREES ==================================

/**
 * This is a hleper struct which contains the contents of a leaf node.
 */
typedef struct NameLeaf {
    char* name;
    int ind;
} NameLeaf;

/**
 * The lookup tree allows for fast mapping from a dictionary of names (stored as char* strings) to associated indices (stored as integers). Lookup speeds are fast, but insertion and removal somewhat slow and are better suited for smaller lists of names
 */
typedef struct sNameTree {
    //if ord(str[i]) <= branch_letter then use child_l, otherwise look at child_r
    char branch_letter;
    _uint branch_ind;//the character index in each string which is used for branching
    NameLeaf* name;//only valid for leaves
    struct sNameTree* child_l;
    struct sNameTree* child_r;
} NameTree;

/**
 * Converts the array of strings of size n_strings with a first element pointed to by strlist into a NameTree. The integer value associated with each string is its position within the array.
 */
NameTree* make_NameTree(size_t n_strings, char** strlist, size_t j, sc_error* err);

/**
 * Searches the NameTree pointed to by dict for the label name.
 * Returns: the associated integer or -1 if no matching name was found
 */
int n_lookup(NameTree* dict, const char* name);

#ifdef __cplusplus 
}
#endif

#endif
