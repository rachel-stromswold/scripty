#include "exec.h"

#ifdef __cplusplus 
extern "C" {
#endif

// ==================================== FUNCTION EXECUTION ====================================

/**
 * Helper function to read the value stored at the stack index ind.
 */
Value _st_fetch(LiveContext* c, size_t ind) {
    return c->callstack.top[ind];
}

/**
 * Execute the already created function f within the runtime Context c.
 */
int _ex_func(Function f, LiveContext* c, DTG_Error* err) {
    InstructionBuffer b = f.buf;
    Value regs[N_REGISTERS];

    //we declare these pointers before the switch statement in which they are used to save on typing
    struct Operation* op = NULL;
    Value* val = NULL;
    Function* fn = NULL;
    HashedItem* hash = NULL;
    //Value* src_val = NULL;
    size_t len = 0;
    size_t ind = 0;
    size_t src_ind = 0;
    size_t dst_ind = 0;

    for (size_t i = 0; i < b.n_insts; ++i) {
	//branch based on the low nibble, note that certain low nibbles may indicate multiple different instructions based on the high nibble, these are listed in comments.
	switch (b.buf[i].i) {
	  //Operation evaluations
	    case INS_OP_EVAL | INS_HH_R:
	  op = (struct Operation*)(regs[b.buf[i+1].i].val.ptr);
	  regs[0] = eval(op, err);
	  i += 2;
	  break;
	    case INS_OP_EVAL | INS_HH_S:
	  op = (struct Operation*)(_st_fetch(c, b.buf[i+1].i).val.ptr);
	  regs[0] = eval(op, err);
	  i += 2;
	  break;
	    case INS_OP_EVAL | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  op = (struct Operation*)(hash->val.val.ptr);
	  regs[0] = eval(op, err);
	  i += 2;
	  break;
	    case INS_OP_EVAL | INS_HH_C:
	  op = (struct Operation*)(b.buf[i+1].ptr);
	  regs[0] = eval(op, err);
	  i += 2;
	  break;

	  //Function Evaluations
	    case INS_FN_EVAL | INS_HH_R:
	  fn = (Function*)(regs[b.buf[i+1].i].val.ptr);
	  regs[0].val.i = _ex_func(*fn, c, err);
	  i += 2;
	  break;
	    case INS_FN_EVAL | INS_HH_S:
	  fn = (Function*)(_st_fetch(c, b.buf[i+1].i).val.ptr);
	  regs[0].val.i = _ex_func(*fn, c, err);
	  i += 2;
	  break;
	    case INS_FN_EVAL | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  fn = (Function*)(hash->val.val.ptr);
	  regs[0].val.i = _ex_func(*fn, c, err);
	  i += 2;
	  break;
	    case INS_FN_EVAL | INS_HH_C:
	  fn = (Function*)(b.buf[i+1].ptr);
	  regs[0].val.i = _ex_func(*fn, c, err);
	  i += 2;
	  break;
	  
	  //Value initialization functions
	  //TODO: make sure that pointers remain valid
	    /*case INS_MAKE_PTR | INS_HH_S:
	  regs[0].val. = &(c->callstack.buf[c->callstack.stack_ptr - b.buf[i+1].i]);
	  i += 2;
	  break;
	    case INS_MAKE_PTR | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  regs[0] = &(hash->val);
	  i += 2;
	  break;
	    case INS_MAKE_ARR:
	  len = b.buf[i+1].i;
	  Value tmplt = {0};
	  regs[0] = v_make_array_n(len, tmplt, err);
	  i += 2;
	  break;
	    case INS_MAKE_STR:
	  len = b.buf[i+1].i;
	  regs[0] = v_make_array_n(len, VT_ERROR, err);
	  i += 2;
	  break;*/

	  //jumps (conditional and unctionditional
	    case INS_JUMP:
	  i = b.buf[i+1].i;
	  break;
	    case INS_JUMP_CND | INS_HH_R:
	  op = (struct Operation*)(regs[b.buf[i+1].i].val.ptr);
	  regs[0] = eval(op, err);
	  if (regs[0].val.i) {
	      i = b.buf[i+2].i;
	  } else {
	      i += 3;
	  }
	  break;
	    case INS_JUMP_CND | INS_HH_S:
	  op = (struct Operation*)(_st_fetch(c, b.buf[i+1].i).val.ptr);
	  regs[0] = eval(op, err);
	  if (regs[0].val.i) {
	      i = b.buf[i+2].i;
	  } else {
	      i += 3;
	  }
	  break;
	    case INS_JUMP_CND | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  op = (struct Operation*)(hash->val.val.ptr);
	  regs[0] = eval(op, err);
	  if (regs[0].val.i) {
	      i = b.buf[i+2].i;
	  } else {
	      i += 3;
	  }
	  break;
	    case INS_JUMP_CND | INS_HH_C:
	  op = (struct Operation*)(b.buf[i+1].ptr);
	  regs[0] = eval(op, err);
	  if (regs[0].val.i) {
	      i = b.buf[i+2].i;
	  } else {
	      i += 3;
	  }
	  break;

	  //Push instructions
	    case INS_PUSH | INS_HH_R:
	  ind = b.buf[i+1].i;
	  push(&(c->callstack), regs[ind], err);
	  i += 2;
	  break;
	    case INS_PUSH | INS_HH_S:
	  push(&(c->callstack), _st_fetch(c, b.buf[i+1].i), err);
	  i += 2;
	  break;
	    case INS_PUSH | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  push(&(c->callstack), hash->val, err);
	  i += 2;
	  break;
	    case INS_PUSH | INS_HH_C:
	  push(&(c->callstack), *( (Value*)(b.buf[i+1].ptr) ), err);
	  i += 2;
	  break;

	  //Pop instructions
	  case INS_POP | INS_HH_R:
	  ind = b.buf[i+1].i;
	  regs[ind] = pop(&(c->callstack), err);
	  i+= 2;
	  break;
	  case INS_POP | INS_HH_S:
	  ind = b.buf[i+1].i;
	  Value tmp = pop(&(c->callstack), err);
	  c->callstack.top[ind] = tmp;
	  i += 2;
	  break;
	  case INS_POP | INS_HH_G:
	  tmp = pop(&(c->callstack), err);
	  break;
	  case INS_POP | INS_HH_C:
	  tmp = pop(&(c->callstack), err);
	  break;

	  //Move instructions
	  case INS_MOV | INS_HH_R | INS_HL_R:
	  dst_ind = b.buf[i+1].i;
	  src_ind = b.buf[i+2].i;
	  regs[dst_ind] = regs[src_ind];
	  break;
	  case INS_MOV | INS_HH_S | INS_HL_R:
	  dst_ind = b.buf[i+1].i;
	  src_ind = b.buf[i+2].i;
	  c->callstack.top[dst_ind] = regs[src_ind];
	  break;
	  case INS_MOV | INS_HH_G | INS_HL_R:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  src_ind = b.buf[i+2].i;
	  hash->val = regs[src_ind];
	  break;
	  case INS_MOV | INS_HH_R | INS_HL_S:
	  dst_ind = b.buf[i+1].i;
	  src_ind = b.buf[i+2].i;
	  regs[dst_ind] = c->callstack.top[src_ind];
	  break;
	  case INS_MOV | INS_HH_S | INS_HL_S:
	  dst_ind = b.buf[i+1].i;
	  src_ind = b.buf[i+2].i;
	  c->callstack.top[dst_ind] = c->callstack.top[src_ind];
	  break;
	  case INS_MOV | INS_HH_G | INS_HL_S:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  src_ind = b.buf[i+2].i;
	  hash->val = c->callstack.top[src_ind];
	  break;
	  case INS_MOV | INS_HH_R | INS_HL_G:
	  dst_ind = b.buf[i+1].i;
	  hash = lookup( &(c->global), (char*)(b.buf[i+2].ptr) );
	  regs[dst_ind] = hash->val;
	  break;
	  case INS_MOV | INS_HH_S | INS_HL_G:
	  dst_ind = b.buf[i+1].i;
	  hash = lookup( &(c->global), (char*)(b.buf[i+2].ptr) );
	  c->callstack.top[dst_ind] = hash->val;
	  break;
	  case INS_MOV | INS_HH_G | INS_HL_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  HashedItem* src_hash = lookup( &(c->global), (char*)(b.buf[i+2].ptr) );
	  hash->val = src_hash->val;
	  break;
	  case INS_MOV | INS_HH_R | INS_HL_C:
	  dst_ind = b.buf[i+1].i;
	  Value* src_val = (Value*)(b.buf[i+2].ptr);
	  regs[dst_ind] = *src_val;
	  break;
	  case INS_MOV | INS_HH_S | INS_HL_C:
	  dst_ind = b.buf[i+1].i;
	  src_val = (Value*)(b.buf[i+2].ptr);
	  c->callstack.top[dst_ind] = *src_val;
	  break;
	  case INS_MOV | INS_HH_G | INS_HL_C:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  src_val = (Value*)(b.buf[i+2].ptr);
	  hash->val = *src_val;
	  break;

	  //pointer dereference
	  case INS_PTR_DRF | INS_HH_R:
	  ind = b.buf[i+1].i;
	  if (regs[ind].type & LO_NIB == VT_REF) {
	      //if the top bit is set then this is a pointer to a global value
	      if (regs[ind].type & TOP_BIT) {
		  hash = lookup( &(c->global), (char*)(regs[ind].val.ptr) );
		  regs[0] = hash->val;
	      } else {
		  ind = regs[ind].val.i;
		  regs[0] = c->callstack.top[ind];
	      }
	  } else {
	      DTG_SetError(err, BADTYPE, "");
	      snprintf(err->msg, DTG_MAX_MSG_SIZE, "Tried to dereference non pointer type %d", regs[ind].type);
	  }
	  break;
	  case INS_PTR_DRF | INS_HH_S:
	  ind = b.buf[i+1].i;
	  if (c->callstack.top[ind].type & LO_NIB == VT_REF) {
	      //if the top bit is set then this is a pointer to a global value
	      if (regs[ind].type & TOP_BIT) {
		  hash = lookup( &(c->global), (char*)(c->callstack.top[ind].val.ptr) );
		  regs[0] = hash->val;
	      } else {
		  ind = regs[ind].val.i;
		  regs[0] = c->callstack.top[ind];
	      }
	  } else {
	      DTG_SetError(err, BADTYPE, "");
	      snprintf(err->msg, DTG_MAX_MSG_SIZE, "Tried to dereference non pointer type %d", regs[ind].type);
	  }
	  break;
	  case INS_PTR_DRF | INS_HH_G:
	  ind = b.buf[i+1].i;
	  if (c->callstack.top[ind].type & LO_NIB == VT_REF) {
	      //if the top bit is set then this is a pointer to a global value
	      if (regs[ind].type & TOP_BIT) {
		  hash = lookup( &(c->global), (char*)(c->callstack.top[ind].val.ptr) );
		  regs[0] = hash->val;
	      } else {
		  ind = regs[ind].val.i;
		  regs[0] = c->callstack.top[ind];
	      }
	  } else {
	      DTG_SetError(err, BADTYPE, "");
	      snprintf(err->msg, DTG_MAX_MSG_SIZE, "Tried to dereference non pointer type %d", regs[ind].type);
	  }
	  break;

	  //Get size instructions
	  case INS_GET_SIZE | INS_HH_R:
	  ind = b.buf[i+1].i;
	  regs[0].val.i = ( (Array*)(regs[ind].val.ptr) )->size;
	  break;
	  case INS_GET_SIZE | INS_HH_S:
	  ind = b.buf[i+1].i;
	  regs[0].val.i = ( (Array*)(c->callstack.top[ind].val.ptr) )->size;
	  break;
	  case INS_GET_SIZE | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  regs[0].val.i = ( (Array*)(hash->val.val.ptr) )->size;
	  break;

	  //Read index from array instructions
	  case INS_IND_READ | INS_HH_R:
	  ind = b.buf[i+1].i;
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      regs[0] = ( (Array*)(regs[ind].val.ptr) )->buf[arr_ind];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;
	  case INS_IND_READ | INS_HH_S:
	  ind = b.buf[i+1].i;
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      regs[0] = ( (Array*)(c->callstack.top[ind].val.ptr) )->buf[arr_ind];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;
	  case INS_IND_READ | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      regs[0] = ( (Array*)(hash->val.val.ptr) )->buf[arr_ind];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;

	  //Write to array index instructions
	  case INS_IND_WRITE | INS_HH_R:
	  ind = b.buf[i+1].i;
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      ( (Array*)(regs[ind].val.ptr) )->buf[arr_ind] = regs[0];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;
	  case INS_IND_WRITE | INS_HH_S:
	  ind = b.buf[i+1].i;
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      ( (Array*)(c->callstack.top[ind].val.ptr) )->buf[arr_ind] = regs[0];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;
	  case INS_IND_WRITE | INS_HH_G:
	  hash = lookup( &(c->global), (char*)(b.buf[i+1].ptr) );
	  //ensure this is an array
	  if (regs[ind].type == VT_ARRAY) {
	      size_t arr_ind = b.buf[i+2].i;
	      ( (Array*)(hash->val.val.ptr) )->buf[arr_ind] = regs[0];
	  } else {
	      DTG_SetError(err, BADTYPE, "Expected array type for writing");
	      return -1;
	  }
	  break;

	  //TODO:
	  case INS_FL_OPEN:
	  case INS_FL_CLOSE:
	  case INS_FL_READ:
	  case INS_FL_WRITE:

	  //make pointer
	  case INS_MAKE_PTR | INS_HH_S:
	  ind = b.buf[i+1].i;
	  regs[0].type = VT_REF;
	  //store the offset from the BOTTOM of the stack to ensure that pushing and popping won't result in alterations
	  //TODO: make sure this is safe
	  regs[0].val.i = c->callstack.bottom - (c->callstack.top+ind);
	  break;
	  case INS_MAKE_PTR | INS_HH_G:
	  regs[0].type = VT_REF | TOP_BIT;
	  //store the offset from the BOTTOM of the stack to ensure that pushing and popping won't result in alterations
	  //TODO: make sure this is safe
	  regs[0].val.ptr = b.buf[i+1].ptr;
	  break;

	  //make strings and arrays
	  case INS_MAKE_ARR:
	  len = (size_t)(regs[0].val.i);
	  regs[0].type = VT_ARRAY;
	  regs[0].val.ptr = _make_Array(sizeof(Value), len, err);
	  if (err->type != E_SUCCESS) {
	      return -1;
	  }
	  break;
	  case INS_MAKE_STR:
	  len = (size_t)(regs[0].val.i);
	  regs[0].type = VT_STRING;
	  //allocate memory for the string
	  regs[0].val.str = (String*)DTG_malloc(sizeof(String), err);
	  *(regs[0].val.str) = make_String_n(len, err);
	  if (err->type != E_SUCCESS) {
	      return -1;
	  }
	  break;

	  case INS_MAKE_VAL:
	  regs[0].type = b.buf[i+1].i;
	  regs[0].val.i = 0;
	  break;

	  case INS_EXT://TODO
	  case INS_RETURN:
	  return 0;
	  default: break;
	}
    }

    return 0;
}

#ifdef __cplusplus 
}
#endif
