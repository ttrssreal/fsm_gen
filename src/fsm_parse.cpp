#include "fsm.h"
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

fsm_err_t fsm_parse_spec(struct fsm* fsm, const char* spec_fname) {
	FILE* spec_file;
	struct fsm_lexer lexer;

	fsm->st_count = 0;
	fsm->st_cap = _PARSER_INIT_FSM_ST_COUNT;
	fsm->st_list = (fsm_st*)malloc(sizeof(fsm_st) * fsm->st_cap);

	if (!(spec_file = fopen(spec_fname, "r"))) {
		fprintf(stderr, "Error: opening file %s\n", spec_fname);
		return -ERR_FILE_OPEN;
	}

	fsm_lexer_init(&lexer, spec_file);

	fsm_parse(fsm, &lexer);

	fclose(spec_file);

	return 0;
}

fsm_err_t _fsm_parse__init_block(struct fsm* fsm, struct fsm_lexer* lexer) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_I) {
		fprintf(stderr, "Error: expected init block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_INP_TKN) {
		fsm_lexer_rewind(lexer);
		fprintf(stderr, "Error: expected a state name for init block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	size_t st_idx;
	_fsm_parse_reslv_st_idx(fsm, tkn.str, &st_idx);
	fsm->st_init = st_idx;
	return 0;
}

fsm_err_t _fsm_parse__acc_block(struct fsm* fsm, struct fsm_lexer* lexer) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_ACC) {
		fprintf(stderr, "Error: expected accept block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_INP_TKN) {
		fsm_lexer_rewind(lexer);
		fprintf(stderr, "Error: expected a state name for accept block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	size_t st_idx;
	_fsm_parse_reslv_st_idx(fsm, tkn.str, &st_idx);
	fsm->st_acc = st_idx;
	return 0;
}

fsm_err_t _fsm_parse__inp_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_I) {
		fprintf(stderr, "Error: expected input block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_INP_TKN) {
		fsm_lexer_rewind(lexer);
		fprintf(stderr, "Error: expected an `input token` for the input block of the transition\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	tr->inp = (fsm_tkn_t)malloc(sizeof(char) * (strlen(tkn.str) + 1));
	strcpy(tr->inp, tkn.str);
	return 0;
}

fsm_err_t _fsm_parse__dst_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_ST_DST) {
		fprintf(stderr, "Error: expected destination block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_INP_TKN) {
		fsm_lexer_rewind(lexer);
		fprintf(stderr, "Error: expected a state name for the destination of the transition block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	_fsm_parse_reslv_st_idx(fsm, tkn.str, &tr->st_dst);
	return 0;
}

fsm_err_t _fsm_parse__name_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_st* st) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_NAME) {
		fprintf(stderr, "Error: expected name block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_INP_TKN) {
		fsm_lexer_rewind(lexer);
		fprintf(stderr, "Error: expected a state name for name block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	st->name = (fsm_tkn_t)malloc(sizeof(char) * (strlen(tkn.str) + 1));
	strcpy(st->name, tkn.str);
	return 0;
}

fsm_err_t _fsm_parse__trans_block(struct fsm* fsm, struct fsm_lexer* lexer, struct fsm_tr* tr) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_TRANS) {
		fprintf(stderr, "Error: expected transition block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_err_t err = _fsm_parse__inp_block(fsm, lexer, tr);
	if (err < 0) {
		fprintf(stderr, "Error: inp block");
		fsm_lexer_rewind(lexer);
		return err;
	}
	err = _fsm_parse__dst_block(fsm, lexer, tr);
	if (err < 0)
		return err;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_CLS_BLK) {
		fprintf(stderr, "Error: expected closing block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	return 0;
}

fsm_err_t _fsm_parse_st__grow_tr_list(struct fsm_st* st) {
	st->tr_cap *= 2;
	void* res = realloc(st->tr_list, sizeof(fsm_tr) * st->tr_cap);
	if (!res) {
		fprintf(stderr, "Error: _fsm_parse_st__grow_tr_list()\n");
		return -ERR_ALLOC;
	}
	st->tr_list = (fsm_tr*)res;
	return 0;
}

fsm_err_t _fsm_parse__state_contents(struct fsm* fsm, struct fsm_lexer* lexer) {
	struct fsm_st tmp_st;

	tmp_st.tr_count = 0;
	tmp_st.tr_cap = _PARSER_INIT_ST_TR_COUNT;
	tmp_st.tr_list = (fsm_tr*)malloc(sizeof(fsm_tr) * tmp_st.tr_cap);

	struct fsm_st* st_ptr = &tmp_st;

	struct fsm_st** active_st = &st_ptr;

	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);

	while (tkn.type == FSM_NAME || tkn.type == FSM_TRANS) {
		switch (tkn.type) {
			// as soon as we see a name, we resolve the state to an array member
			case FSM_NAME: {
				fsm_err_t err = _fsm_parse__name_block(fsm, lexer, *active_st);
				if (err < 0)
					return err;

				size_t st_idx;
				_fsm_parse_reslv_st_idx(fsm, (*active_st)->name, &st_idx);
				fsm_st* st = &fsm->st_list[st_idx];

				// populate it with the state weve been building
				st->tr_count = (*active_st)->tr_count;
				st->tr_cap = (*active_st)->tr_cap;
				st->tr_list = (*active_st)->tr_list;

				// swap the state ptr to the new state
				*active_st = st;

				fsm_lexer_peek_tkn(lexer, &tkn);
				continue;
			}
			case FSM_TRANS: {
				if ((*active_st)->tr_count >= (*active_st)->tr_cap)
					_fsm_parse_st__grow_tr_list(*active_st);
	
				fsm_tr* tr = &(*active_st)->tr_list[(*active_st)->tr_count++];
	
				fsm_err_t err = _fsm_parse__trans_block(fsm, lexer, tr);
				if (err < 0)
					return err;
				fsm_lexer_peek_tkn(lexer, &tkn);
				continue;
			}
			default: {
				fprintf(stderr, "Error: expected state contents\n");
				return -ERR_PARSE;
			}
		}
		fsm_lexer_peek_tkn(lexer, &tkn);
	}
	return 0;
}

fsm_err_t _fsm_parse__state_block(struct fsm* fsm, struct fsm_lexer* lexer) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_STATE) {
		fprintf(stderr, "Error: expected state block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	fsm_err_t err = _fsm_parse__state_contents(fsm, lexer);
	if (err < 0)
		return err;
	fsm_lexer_peek_tkn(lexer, &tkn);
	if (tkn.type != FSM_CLS_BLK) {
		fprintf(stderr, "Error: expected closing block\n");
		return -ERR_PARSE;
	}
	fsm_lexer_eat_tkn(lexer);
	return 0;
}

fsm_err_t _fsm_parse_reslv_st_idx(struct fsm* fsm, char* st_name, size_t* st_idx) {
	for (size_t i = 0; i < fsm->st_count; ++i) {
		if (!strcmp(fsm->st_list[i].name, st_name)) {
			*st_idx = i;
			return 0;
		}
	}

	if (fsm->st_count >= fsm->st_cap) {
		fsm->st_cap *= 2;
		void* res = realloc(fsm->st_list, sizeof(fsm_st) * fsm->st_cap);
		if (!res) {
			fprintf(stderr, "Error: _fsm_parse__reslv_st_idx()\n");
			return -ERR_ALLOC;
		}
		fsm->st_list = (fsm_st*)res;
	}

	fsm_st* new_st = &fsm->st_list[fsm->st_count];

	new_st->name = (char*)malloc(strlen(st_name) + 1);
	strcpy(new_st->name, st_name);

	// currently were an invalid state, so we handle allocations later
	new_st->tr_count = 0;
	new_st->tr_cap = 0;
	new_st->tr_list = NULL;

	*st_idx = fsm->st_count++;

	return 0;
}

fsm_err_t fsm_parse(struct fsm* fsm, struct fsm_lexer* lexer) {
	struct fsm_lexer_tkn tkn;
	fsm_lexer_peek_tkn(lexer, &tkn);
	while (tkn.type != FSM_EOF) {
		switch (tkn.type) {
			case FSM_I: {
				fsm_err_t err = _fsm_parse__init_block(fsm, lexer);
				if (err < 0)
					return err;
				fsm_lexer_peek_tkn(lexer, &tkn);
				break;
			}
			case FSM_ACC: {
				fsm_err_t err = _fsm_parse__acc_block(fsm, lexer);
				if (err < 0)
					return err;
				fsm_lexer_peek_tkn(lexer, &tkn);
				break;
			}
			case FSM_STATE: {
				fsm_err_t err = _fsm_parse__state_block(fsm, lexer);
				if (err < 0)
					return err;
				fsm_lexer_peek_tkn(lexer, &tkn);
				break;
			}
			default:
				break;
		}
	}
	return 0;
}

fsm_err_t fsm_init(struct fsm* fsm) {
	fsm->st_list = NULL;
	fsm->st_count = 0;
	return 0;
}


fsm_err_t fsm_release(struct fsm* fsm) {
	free(fsm->st_list);
	return 0;
}

fsm_err_t fsm_dbg_print(struct fsm* fsm) {
	for (size_t st = 0; st < fsm->st_count; ++st) {
		fsm_dbg_print_st(fsm, &fsm->st_list[st]);
	}
	return 0;
}

fsm_err_t fsm_dbg_print_st(struct fsm* fsm, struct fsm_st* st) {
	printf("STATE (%s)%s\n",
			st->name,
			st->tr_count ? " {" : "");
	for (size_t tr = 0; tr < st->tr_count; ++tr) {
		printf("\tTRANSITION[%s] -> (%s)\n",
				st->tr_list[tr].inp,
				RESLV_STATE
					(fsm->st_list,
					st->tr_list[tr].st_dst
				).name
				);
	}
	return 0;
}
