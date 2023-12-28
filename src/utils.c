#include "utils.h"

#ifdef __cplusplus 
extern "C" {
#endif

// ==================================== HELPERS ====================================

/**
 * check if a character is whitespace
 */
int is_whitespace(char c) {
    return (c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

/**
 * returns: whether the character c is the start of a nesting block i.e. (),[],{}
 */
int is_nest_start(char c) {
    return (c == '(' || c == '[' || c == '{');
}

/**
 * returns: whether the character c is the start of a nesting block i.e. (),[],{}
 */
int is_nest_end(char c) {
    return (c == ')' || c == ']' || c == '}');
}

/**
 * returns: whether the character e is the end of a nesting block started with character s, i.e. (),[],{}
 */
int is_nest_match(char s, char e) {
    return ((s == '(' && e == ')') || (s == '[' && e == ']') || (s == '{' && e == '}'));
}

/**
 * Helper function for is_token which tests whether the character c is a token terminator
 */
int is_char_sep(char c) {
    return (is_nest_start(c) || is_nest_end(c) || is_whitespace(c) || c == ';' || c == '+'  || c == '-' || c == '*');
}

/**
 * Helper function which looks at the string str at index i and tests whether it is a token with the matching name
 */
int is_token(const char* str, size_t i, size_t len) {
    if (i > 0 && !is_char_sep(str[i-1]))
	return 0;
    if (!is_char_sep(str[i+len]))
	return 0;
    return 1;
}

/**
 * Interprets the character d as a digit in the specified base. In the event of an invalid character, -1 is returned.
 */
int _read_digit(char d, _uint base) {
    if (base <= 10) {
	if (d >= '0' && d < '0' + base) {
	    return d - '0';
	}
	return -1;
    } else {
	if (d >= '0' && d < '0' + base) {
	    return d - '0';
	} else if (d >= 'a' && d < 'a' + base - 10) {
	    return d - 'a' + 10;
	} else if (d >= 'A' && d < 'A' + base - 10) {
	    return d - 'A' + 10;
	}
	return -1;
    }
}

// ==================================== PARSING ====================================

/**
 * Returns the portion of the string str between the first match from the set of open and close characters provided. Open and close can be strings containing multiple characters in wich case the enclosed contents will follow the first match for open and the matching close character will be used. No heap memory is allocated by this function.
 * NOTE: For multiple matching open and close characters, open and close MUST be of equal length. Furthermore, the match is based on the index within the strings open and closed. For instance a call to _get_enclosed(str, "({", "})") where str="{foo}" will result in an error (NULL is returned). Instead, the user should call _get_enclosed(str, "({", ")}") so that the curly brace is the second character in both the open and close strings.
 * NOTE: this function preserves "scope". That is, both the open and close characters must be in the same scope. For instance _get_enclosed(str, "(", ")") where str="((foo)bar)" will return a string with contents "(foo)bar" instead of "(foo".
 * WARNING: str is modified in place and an address returned. You should duplicate the string before calling this function if you need to use the original string.
 * Returns: the enclosed contents as described above or NULL if an error is encountered. If no matching open character is encountered then the string is returned unaltered.
 */
char* _get_enclosed(char* str, const char* open, const char* close) {
    if (strlen(close) != strlen(open)) { return NULL; }
    size_t i = 0;
    size_t start = 0;
    //store the index of the matching character in open. For open and close containing multiple characters we set close_char to match the opening character we want.
    char close_char = 0;
    char open_char = 0;
    while (close_char == 0) {
	//see if we have a matching character from the open set
	for (size_t j = 0; open[j] != 0; ++j) {
	    if (str[i] == open[j]) {
		open_char = open[j];
		close_char = close[j];
		start = i + 1;
		break;
	    }
	}
	++i;
	//terminate if we reached the end of the string
	if (str[i] == 0) { return str; }
    }
    //this is a special case where nest counting doesn't work (any hit is essentially a no-op) instead we just examine if the end character is escaped
    if (open_char == close_char) {
	int escaped = 0;
	for (i = start; str[i] != 0; ++i) {
	    //see if the next character should be escaped by setting escaped = NOT escaped
	    if (str[i] == '\\') {
		escaped = 1 - escaped;
	    }
	    //see if we have a matching character from the open set and try incrementing
	    if (str[i] == close_char && !escaped) {
		str[i] = 0;
		return str + start;
	    }
	}
    } else {
	int nest_level = 0;
	for (i = start; str[i] != 0; ++i) {
	    //see if we have a matching character from the open set and try incrementing
	    for (size_t j = 0; open[j] != 0; ++j) {
		if (str[i] == open[j]) { ++nest_level; }
	    }
	    //see if we have a matching character from the open set and try incrementing
	    if (str[i] == close_char) {
		--nest_level;
		//we check that the nest level is less than 0 since 0 is the nest level INSIDE the parenthesis
		if (nest_level < 0) {
		    str[i] = 0;
		    return str + start;
		}
	    }
	}
    }
    return str + start;
}

char* _get_enclosed_r(char* str, char** saveptr, const char* open, const char* close) {
    if (strlen(close) != strlen(open)) { return NULL; }
    if (saveptr) { *saveptr = NULL; }
    size_t i = 0;
    size_t start = 0;
    //store the index of the matching character in open. For open and close containing multiple characters we set close_char to match the opening character we want.
    char close_char = 0;
    char open_char = 0;
    while (close_char == 0) {
	//see if we have a matching character from the open set
	for (size_t j = 0; open[j] != 0; ++j) {
	    if (str[i] == open[j]) {
		open_char = open[j];
		close_char = close[j];
		start = i + 1;
		break;
	    }
	}
	++i;
	//terminate if we reached the end of the string
	if (str[i] == 0) { return str; }
    }
    //this is a special case where nest counting doesn't work (any hit is essentially a no-op) instead we just examine if the end character is escaped
    if (open_char == close_char) {
	int escaped = 0;
	for (i = start; str[i] != 0; ++i) {
	    //see if the next character should be escaped by setting escaped = NOT escaped
	    if (str[i] == '\\') {
		escaped = 1 - escaped;
	    }
	    //see if we have a matching character from the open set and try incrementing
	    if (str[i] == close_char && !escaped) {
		str[i] = 0;
		if (saveptr) { *saveptr = str+i+1; }
		return str + start;
	    }
	}
    } else {
	int nest_level = 0;
	for (i = start; str[i] != 0; ++i) {
	    //see if we have a matching character from the open set and try incrementing
	    for (size_t j = 0; open[j] != 0; ++j) {
		if (str[i] == open[j]) { ++nest_level; }
	    }
	    //see if we have a matching character from the open set and try incrementing
	    if (str[i] == close_char) {
		--nest_level;
		//we check that the nest level is less than 0 since 0 is the nest level INSIDE the parenthesis
		if (nest_level < 0) {
		    str[i] = 0;
		    if (saveptr) { *saveptr = str+i+1; }
		    return str + start;
		}
	    }
	}
    }
    if (saveptr) { *saveptr = NULL; }
    return str + start;
}

/**
 * Get the first instance of the token tok after index i in the string str which is at the same nest level. i.e search_block("a(b[1,2],c),3", ",", 0) returns 11 (the index of the ',' before the 3)
 * str: the string to search through
 * tok: the token to look for
 * i: the index to start the search
 * returns: the index of the first occurence of the string or SIZE_MAX if the token was not found (note the use of unsigned size_t causes underflows so that this function will break if trying to parse a string with more than ~9.2x10^18 characters (so you probably don't have to worry lol.
 */
size_t _search_block(const char* str, const char* tok, size_t i) {
    //for a stack with fixed depth
    size_t blk_stk[MAX_BLK_RECURSE];
    size_t st_ptr = 0;
    for (size_t j=i; str[j] != 0; ++j) {
	//check if the stack is empty, meaning we're at the root nest level
	if (st_ptr == 0) {
	    //this means we've reached the end of this nest level without finding anything
	    if (is_nest_end(str[j]))
		return INONE;

	    //check that we have a match
	    size_t k = 0;
	    while (tok[k] && str[j+k] == tok[k])
		++k;

	    //check that the match is actually a token (i.e. surrounded by whitespace or nests)
	    if (tok[k] == 0 && is_token(str, j, k))
		return j;
	} else {
	    //otherwise, check if we've reached the end of a nest (pop off the stack)
	    if ( is_nest_match(str[blk_stk[st_ptr-1]], str[j]) )
		--st_ptr;
	}
	//push each nest level onto the stack
	if (is_nest_start(str[j])) {
	    blk_stk[st_ptr++] = j;
	    //check for overflows
	    if (st_ptr == MAX_BLK_RECURSE) {
		printf("Error while parsing: maximum nest depth exceeded.\n");
		return INONE;
	    }
	}
    }
    return INONE;
}

/**
 * Convert a string separated by the character sep into a list of strings. For instance, if str == "arg1, arg2" and sep == ',' then the output will be a list of the form ["arg1", "arg2"]. If no sep character is found then the list will have one element which contains the whole string.
 * param str: string to parse into a list
 * param sep: separating character to use.
 * param listlen: location to save the length of the returned list to. Note that this pointer MUST be valid. It is not acceptable to call this function with listlen = NULL. (The returned list is not null terminated so this behavior ensures that callers are appropriately able to identify the length of the returned string.)
 * returns: list of strings separated by commas. This should be freed with a call to sc_free(). In the event of an error, NULL is returned instead.
 * NOTE: The input string str is modified and the returned value uses memory allocated for str. Thus, the caller must ensure that str has a lifetime at least as long as the used string. Users should not try to call free() on any of the strings in the list, only the list itself.
 */
char** csv_to_list(char* str, char sep, size_t* listlen, sc_error* err) {
    char** ret = NULL;/*(char**)sc_malloc(sizeof(char*), &tmp_err);*/
    if (err->type != E_SUCCESS) { return NULL; }
    //we don't want to include separators that are in nested environments i.e. if the input is [[a,b],c] we should return "[a,b]","c" not "[a","b]","c"
    blk_type tp_stk[MAX_BLK_RECURSE];
    size_t st_ptr = 0;

    //by default we ignore whitespace, only use it if we are in a block enclosed by quotes
    int verbatim = 0;
    char* saveptr = str;
    size_t off = 0;
    size_t j = 0;
    for (size_t i = 0; str[i] != 0; ++i) {
	//if this is a separator then add the entry to the list
	if (str[i] == sep && st_ptr == 0) {
	    ret = (char**)sc_realloc(ret, sizeof(char*)*(off+1), err);
	    if (err->type != E_SUCCESS) { return NULL; }

	    //append the element to the list
	    ret[off] = saveptr;
	    ++off;
	    //null terminate this string and increment j
	    str[j] = 0;
	    ++j;
	    saveptr = str + j;
	}
	//check for escape sequences
	if (str[i] == '\\') {
	    ++i;
	    switch (str[i]) {
	    case 'n': str[j] = '\n';++j;break;
	    case 't': str[j] = '\t';++j;break;
	    case '\\': str[j] = '\\';++j;break;
	    case '\"': str[j] = '\"';++j;break;
	    default: sc_set_error(err, E_SYNTAX, "unrecognized escape sequence");
	    }
	} else if (str[i] == '\"') {
	    //if this is an unescaped quote then toggle verbatim mode
	    verbatim = 1 - verbatim;
	} else if (str[i] == '['/*]*/) {
	    tp_stk[st_ptr++] = BLK_SQUARE;
	} else if (str[i] == /*[*/']') {
	    if (st_ptr == 0 || tp_stk[--st_ptr] != BLK_SQUARE) {
		free(ret);
		sc_set_error(err, E_SYNTAX, strdup(/*[*/"Unexpected ']'"));
		return NULL;
	    }
	} else if (str[i] == '('/*)*/) {
	    tp_stk[st_ptr++] = BLK_PAREN;
	} else if (str[i] == /*(*/')') {
	    if (st_ptr == 0 || tp_stk[--st_ptr] != BLK_PAREN) {
		free(ret);
		sc_set_error(err, E_SYNTAX, strdup(/*(*/"Unexpected ')'"));
		return NULL;
	    }
	} else if (str[i] == '{'/*}*/) {
	    tp_stk[st_ptr++] = BLK_CURLY;
	} else if (str[i] == /*{*/'}') {
	    if (st_ptr == 0 || tp_stk[--st_ptr] != BLK_CURLY) {
		free(ret);
		sc_set_error(err, E_SYNTAX, strdup(/*{*/"Unexpected '}'"));
		return NULL;
	    }
	} else if (verbatim ||
		  (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')) {
	    //if this isn't whitespace or it is verbatim mode then just copy the character
	    str[j] = str[i];
	    ++j;
	}
    }
    if (listlen) {
	*listlen = off + 1;
    }
    ret = (char**)sc_realloc(ret, sizeof(char*)*(off+1), err);
    ret[off] = NULL;
    return ret;
}

/**
 * This function modifies the string in place removing leading and trailing whitespace.
 */
char* _trim_whitespace(char* str) {
    //keep track of whether we are in the string or not
    int started = 0;

    size_t first_valid_char = 0;
    size_t last_valid_char = 0;
    for (size_t i = 0; str[i] != 0; ++i) {
	if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
	    if (!started) {
		first_valid_char = i;
		started = 1;
	    }
	    last_valid_char = i;
	}
    }
    str[last_valid_char + 1] = 0;
    return str + first_valid_char;
}

/**
 * Compares the next (non whitespace) word in the string str with the token tok. This word is terminated by either a whitespace character or a null terminator.
 * If the comparison evaluates to an equivalence a positive integer representing the number of needed to skip to reach the END of the matched token. Otherwise -1 is returned. If there is no next word, 0 is returned. In the event of an error, -2 is returned.
 */
int cmpnwrd(const char* str, const char* tok) {
    if (str == NULL) { return -2; }

    //iterate until we find the next word (indicated by a non whitespace character
    int i = 0;
    for (; str[i] == ' ' && str[i] == '\t' && str[i] == '\n'; ++i) {}
    if (str[i] == 0) { return 0; }

    //iterate over the token and return -1 if the word is not a match
    int j = 0;
    for (; tok[j] != 0; ++j) {
	//check to ensure we haven't hi the end of str and check the characters.
	if (str[i+j] == 0 || str[i+j] != tok[j]) { return -1; }
    }
    return i+j;
}

/**
 * Reads the next whole word (sequence of non whitespace characters) from string str starting at offset off. Up to max_n bytes from this word are read into the string sto (sto is guaranteed to be null terminated). Returns an index (relative to str+off) to the first character AFTER the word that was just read. If there are no remaining words then 0 is returned. In the event that max_n is not large enough to hold the read word the index to the first unread character is returned and the remaining portion of the word may be accessed through one or more successive calls to read_word().
 */
size_t read_dtg_word(const char* str, size_t off, char* sto, size_t max_n) {
     //iterate until we find the next word (indicated by a non whitespace character
    int i = off;
    for (; str[i] != 0 && (str[i] == ' ' || str[i] == '\t' || str[i] == '\n'); ++i) {}
    if (str[i] == 0) { return 0; }

    //single character words that represent the start or end of a parenthetical expression or an assignment character are a special case and should be treated as a whole word
    if (str[i] == '=' || str[i] == ',' ||//special dtg characters
	str[i] == '(' || str[i] == ')' || str[i] == '{' || str[i] == '}' ||
	str[i] == '[' || str[i] == ']') {//parenthesis
	sto[0] = str[i];
	sto[1] = 0;
	return i+1;
    }

    //read the word into sto
    for (size_t j = 0; j < max_n; ++j) {
	sto[j] = str[i];
	if (str[i] == 0) { return i; }
	if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t' ||//whitespace
	    str[i] == ';' || str[i] == '=' ||//special dtg characters
	    str[i] == '(' || str[i] == ')' || str[i] == '{' || str[i] == '}' ||
	    str[i] == '[' || str[i] == ']') {//parenthesis
	    sto[j] = 0;
	    return i+1;
	}
	++i;
    }

    sto[max_n-1] = 0;
    return i+1;
}

/**
 * Reads the next whole word (sequence of non whitespace characters) from string str starting at offset off. This is performed "in place" modifying the string str. The resulting string is saved to *sto if sto is not NULL. The caller must ensure that the lifespan of sto does not exceed that of str.
 */
size_t read_word_i(char* str, size_t off, char** sto) {
     //iterate until we find the next word (indicated by a non whitespace character
    int i = off;
    for (; str[i] != 0 && (str[i] == ' ' || str[i] == '\t' || str[i] == '\n'); ++i) {}
    if (str[i] == 0) { return 0; }

    //read the word into sto
    if (sto) { *sto = str+i; }
    for (; str[i] != 0; ++i) {
	if (str[i] == 0) { return i; }
	if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t') {
	    str[i] = 0;
	    return i+1;
	}
    }

    return i+1;
}

/**
 * Creates a new string with memory allocated through sc_malloc. The contents of s are copied into this memory. If CONSERVE_MEM is defined, then the memory is resized to exactly hold the specified contents. In the event of an error, NULL is returned.
 */
char* DTG_strdup(const char* s, sc_error* err) {
    //allocate memory for the return string
    size_t cap = DEFAULT_WORD_SIZE;
    char* ret = (char*)sc_malloc(sizeof(char)*cap, err);
    if (err->type != E_SUCCESS) { return NULL; }
    
    //iterate over s
    size_t i = 0;
    while (1) {
	//expand memory if necessary
	if (i == cap) {
	    cap *= 2;
	    ret = (char*)sc_realloc(ret, cap, err);
	    if (err->type != E_SUCCESS) { sc_free(ret);return NULL; }
	}
	ret[i] = s[i];
	if (s[i] == 0) { break; }
	++i;
    }
#ifdef CONSERVE_MEM
    ret = (char*)sc_realloc(ret, i+1, err);
#endif
    return ret;
}

// ================================== LOOKUP TREES ==================================

/**
 * Converts the array of strings of size n_strings with a first element pointed to by strlist into a NameTree. The integer value associated with each string is its position within the array.
 * param n_strings: the number of strings in the array
 * param strlist: the array of strings
 * param j: The offset character into each string which is used for sorting. For instance, if j=1, then children will be generated by comparing the second character in each string with the branch. (j=0 would correspond to the first because we use zero-indexing)
 */
NameTree* make_NameTree(size_t n_strings, char** strlist, size_t j, sc_error* err) {
    //base case, return NULL
    if (n_strings == 0) {
	return NULL;
    }

    NameTree* ret = (NameTree*)sc_malloc(sizeof(NameTree), err);
    if (n_strings == 1) {
	ret->name = (NameLeaf*)sc_malloc(sizeof(NameLeaf), err);
	ret->name->name = strlist[0];
	ret->name->ind = -1;//to be filled in later
    }
    ret->name = NULL;
    if (err->type != E_SUCCESS) { free(ret);return NULL; }
    /* TODO: implement median using quickselect
    char min = SCHAR_MAX;
    char max = SCHAR_MIN;
    size_t med_ind = 0;
    for (size_t i = 0; i < n_strings; ++i) {
	if (strlist[i][0] < min) { min = strlist[i][0]; }
	if (strlist[i][0] > max) { max = strlist[i][0]; }
    }*/
    //as a heuristic set the branch letter to the average of the first character from each string
    int sum = 0; 
    for (size_t i = 0; i < n_strings; ++i) {
	sum += (int)(strlist[i][j]);
    }
    ret->branch_letter = (char)(sum / n_strings);
    //Create two new lists. The first list will contain strings which have a first character less than or equal to the branch character and the second will contain first characters greater than this value. We remove this first character and only include the rest of the string.
    char** next_strings = (char**)sc_malloc(sizeof(char*)*n_strings, err);
    if (err->type != E_SUCCESS) { free(ret);return NULL; }
    size_t n_lows = 0;
    size_t n_highs = 0;
    for (size_t i = 0; i < n_strings; ++i) {
	//depending on the character add it to the high or low list
	if (strlist[i][j] <= ret->branch_letter) {
	    next_strings[n_lows] = strlist[i] + 1;
	    n_lows += 1;
	} else {
	    n_highs += 1;
	    next_strings[n_strings - n_highs] = strlist[i] + 1;
	}
    }
    //recursively make the children and check for errors
    ret->child_l = make_NameTree(n_lows, next_strings, j+1, err);
    if (err->type != E_SUCCESS) {
	free(ret->child_l);
	free(ret);
	return NULL;
    }
    ret->child_r = make_NameTree(n_highs, next_strings + n_lows, j+1, err);
    if (err->type != E_SUCCESS) {
	free(ret->child_l);
	free(ret->child_r);
	free(ret);
	return NULL;
    }
    return ret;
}

/**
 * Searches the NameTree pointed to by dict for the label name.
 * Returns: the associated integer or -1 if no matching name was found
 */
int n_lookup(NameTree* dict, const char* name);

#ifdef __cplusplus 
}
#endif
