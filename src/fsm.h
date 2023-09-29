#pragma once

#include <cstddef>
#include <stdlib.h>
#include <stdio.h>

typedef int fsm_err_t;
typedef size_t fsm_st_ref_t;
typedef char* fsm_tkn_t;

#define ERR_FILE_OPEN 1
#define ERR_PARSE_SPEC_LINE 3
#define ERR_RESLV_ST_NAME 4
#define ERR_ALLOC 5
#define ERR_PARSE 6

#define _LEXER_INIT_TKN_COUNT 16

#define _PARSER_INIT_ST_TR_COUNT 16
#define _PARSER_INIT_FSM_ST_COUNT 16

#define _CODEGEN_PROLOUGE_FNAME "prolouge.def"
#define _CODEGEN_EPILOUGE_FNAME "epilouge.def"

#define FSM_UNDEF_STATE -1
#define RESLV_STATE(st_list, st) st_list[st]

struct fsm;
struct fsm_tr;
struct fsm_st;

struct fsm_tr {
	fsm_tkn_t inp;
	fsm_st_ref_t st_dst;
};

struct fsm_st {
	char* name;
	fsm_tr* tr_list;
	size_t tr_count;
	size_t tr_cap;
};

struct fsm {
	fsm_st* st_list;
	size_t st_cap;
	size_t st_count;
	fsm_st_ref_t st_init;
	fsm_st_ref_t st_acc;
};

struct fsm_lexer;
struct fsm_lexer_tkn;

struct fsm_lexer {
	FILE* spec_file;
	struct fsm_lexer_tkn* tkn_list;
	size_t tkn_cap;
	size_t tkn_count;
	size_t tkn_idx;
	char c_cur;
	char c_next;
};

typedef enum fsm_lexer_tkn_type {
	FSM_STATE, // `s{`
	FSM_ACC, // `a{`
	FSM_I, // `i{`
	FSM_NAME, // `n{`
	FSM_TRANS, // `t{`
	FSM_ST_DST, // `d{`
	FSM_CLS_BLK, // `}`
	FSM_INP_TKN,
	FSM_EOF,
} fsm_lexer_tkn_type_t;

struct fsm_lexer_tkn {
	fsm_lexer_tkn_type_t type;
	char* str;
};

static const char* fsm_lexer_tkn_nm_lookup[] = {
	"STATE",
	"ACC",
	"I",
	"NAME",
	"TRANS",
	"ST_DST",
	"CLS_BLK",
	"INP_TKN",
	"EOF",
};


fsm_err_t fsm_init(struct fsm* fsm);
fsm_err_t fsm_release(struct fsm* fsm);
fsm_err_t fsm_parse_spec(struct fsm* fsm, const char* spec_fname);

fsm_err_t fsm_parse(struct fsm* fsm, struct fsm_lexer* lexer);

// recursive decent parser methods (correspond approximately to productions in the grammar)
fsm_err_t _fsm_parse__init_block(struct fsm* fsm, struct fsm_lexer* lexer);
fsm_err_t _fsm_parse__acc_block(struct fsm* fsm, struct fsm_lexer* lexer);
fsm_err_t _fsm_parse__inp_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr);
fsm_err_t _fsm_parse__dst_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr);
fsm_err_t _fsm_parse__name_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_st* st);
fsm_err_t _fsm_parse__trans_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr);
fsm_err_t _fsm_parse__state_contents(struct fsm* fsm, struct fsm_lexer* lexer);
fsm_err_t _fsm_parse__state_block(struct fsm* fsm, struct fsm_lexer* lexer);
fsm_err_t _fsm_parse__fsm_with_init(struct fsm* fsm, struct fsm_lexer* lexer);

fsm_err_t _fsm_parse_st__grow_tr_list(struct fsm_st* st);

fsm_err_t _fsm_parse_reslv_st_idx(struct fsm* fsm, char* st_name, size_t* st_idx);

fsm_err_t fsm_lexer_init(struct fsm_lexer* lexer, FILE* spec_file);
fsm_err_t fsm_lexer_release(struct fsm_lexer* lexer);
fsm_err_t fsm_lexer_eat_tkn(struct fsm_lexer* lexer);
fsm_err_t fsm_lexer_peek_tkn(struct fsm_lexer* lexer, struct fsm_lexer_tkn* tkn);
fsm_err_t fsm_lexer_rewind(struct fsm_lexer* lexer);
fsm_err_t fsm_lexer_rewind_n(struct fsm_lexer* lexer, size_t n);

fsm_err_t _fsm_lexer_update_chars(struct fsm_lexer* lexer);
fsm_err_t _fsm_lexer_match_tkn(struct fsm_lexer* lexer, struct fsm_lexer_tkn* tkn);
fsm_err_t _fsm_lexer_grow_tkn_list(struct fsm_lexer* lexer);
fsm_err_t _fsm_lexer_read_tkn(struct fsm_lexer* lexer);

fsm_err_t fsm_codegen(struct fsm* fsm, char*& generated);
fsm_err_t fsm_codegen_gen_state(struct fsm* fsm, struct fsm_st* st, char*& out);

fsm_err_t fsm_dbg_print(struct fsm* fsm);
fsm_err_t fsm_dbg_print_st(struct fsm* fsm, struct fsm_st* st);
