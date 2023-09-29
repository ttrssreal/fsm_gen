#include "fsm.h"
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdio.h>

fsm_err_t fsm_lexer_init(struct fsm_lexer* lexer, FILE* spec_file) {
	lexer->spec_file = spec_file;
	lexer->tkn_count = 0;
	lexer->tkn_idx = 0;
	lexer->tkn_cap = _LEXER_INIT_TKN_COUNT;

	void* res = malloc(sizeof(struct fsm_lexer_tkn) * lexer->tkn_cap);
	if (!res) {
		fprintf(stderr, "Error: allocation error\n");
		return -ERR_ALLOC;
	}

	lexer->tkn_list = (struct fsm_lexer_tkn*)res;

	lexer->c_cur = fgetc(spec_file);
	lexer->c_next = fgetc(spec_file);

	_fsm_lexer_read_tkn(lexer);
	_fsm_lexer_read_tkn(lexer);

	return 0;
}

fsm_err_t fsm_lexer_release(struct fsm_lexer* lexer) {
	free(lexer->tkn_list);
	return 0;
}

fsm_err_t _fsm_lexer_update_chars(struct fsm_lexer* lexer) {
	lexer->c_cur = lexer->c_next;
	lexer->c_next = fgetc(lexer->spec_file);
	return 0;
}

fsm_err_t _fsm_lexer_test_whitespace(struct fsm_lexer* lexer) {
	return lexer->c_cur == ' '
		|| lexer->c_cur == '\n'
		|| lexer->c_cur == '\r'
		|| lexer->c_cur == '\t';
}

fsm_err_t _fsm_lexer_match_tkn(struct fsm_lexer* lexer, struct fsm_lexer_tkn* tkn) {
	#define _LEXER_EMIT_BLK_BEGIN(typ) \
		{ \
			tkn->type = typ; \
			tkn->str = NULL; \
			_fsm_lexer_update_chars(lexer); \
		}

	while (_fsm_lexer_test_whitespace(lexer)) {
		_fsm_lexer_update_chars(lexer);
	}

	if (lexer->c_cur == EOF) {
		tkn->type = FSM_EOF;
		tkn->str = NULL;
		return 0;
	}

	if (lexer->c_cur == '}') {
		tkn->type = FSM_CLS_BLK;
		tkn->str = NULL;
		return 0;
	}

	if (lexer->c_next == '{') {
		switch (lexer->c_cur) {
			case 's':
				_LEXER_EMIT_BLK_BEGIN(FSM_STATE);
				return 0;
			case 'a':
				_LEXER_EMIT_BLK_BEGIN(FSM_ACC);
				return 0;
			case 'i':
				_LEXER_EMIT_BLK_BEGIN(FSM_I);
				return 0;
			case 'n':
				_LEXER_EMIT_BLK_BEGIN(FSM_NAME);
				return 0;
			case 't':
				_LEXER_EMIT_BLK_BEGIN(FSM_TRANS);
				return 0;
			case 'd':
				_LEXER_EMIT_BLK_BEGIN(FSM_ST_DST);
				return 0;
		}
	}

	tkn->type = FSM_INP_TKN;
	size_t tkn_str_len = 0;
	size_t tkn_str_cap = 16;
	fsm_tkn_t tkn_str = (fsm_tkn_t)malloc(sizeof(char) * tkn_str_cap);

	while (lexer->c_cur != '}'
			&& !_fsm_lexer_test_whitespace(lexer)
			&& lexer->c_cur != EOF) {
		if (tkn_str_len >= tkn_str_cap - 1) {
			tkn_str_cap *= 2;
			void* res = realloc(tkn_str, sizeof(char) * tkn_str_cap);
			if (!res) {
				fprintf(stderr, "Error: fsm_lexer_match_tkn()\n");
				return -ERR_ALLOC;
			}
			tkn_str = (fsm_tkn_t)res;
		}
		tkn_str[tkn_str_len++] = lexer->c_cur;
		_fsm_lexer_update_chars(lexer);
	}

	if (_fsm_lexer_test_whitespace(lexer)) {
		while (_fsm_lexer_test_whitespace(lexer)) {
			_fsm_lexer_update_chars(lexer);
		}
		if (lexer->c_cur != '}') {
			fprintf(stderr, "Error: fsm_lexer_match_tkn()\n");
			return -ERR_PARSE;
		}
	}

	tkn_str[tkn_str_len] = 0;
	tkn->str = tkn_str;

	return 0;
}

fsm_err_t _fsm_lexer_read_tkn(struct fsm_lexer* lexer) {
	if (lexer->tkn_count >= lexer->tkn_cap) {
		fsm_err_t res = _fsm_lexer_grow_tkn_list(lexer);
		if (res) {
			fprintf(stderr, "Error: fsm_lexer_read_token()\n");
			return -ERR_ALLOC;
		}
	}

	struct fsm_lexer_tkn* tkn = &lexer->tkn_list[lexer->tkn_count];
	_fsm_lexer_match_tkn(lexer, tkn);

	++lexer->tkn_count;
	_fsm_lexer_update_chars(lexer);
	return 0;
}

fsm_err_t _fsm_lexer_grow_tkn_list(struct fsm_lexer* lexer) {
	size_t new_cap = lexer->tkn_cap * 2;
	void* res = realloc(lexer->tkn_list, sizeof(struct fsm_lexer_tkn) * new_cap);
	if (!res) {
		fprintf(stderr, "Error: fsm_lexer_grow_tkn_list()\n");
		return -ERR_ALLOC;
	}
	lexer->tkn_list = (struct fsm_lexer_tkn*)res;
	lexer->tkn_cap = new_cap;
	return 0;
}

// primary lexer public api
fsm_err_t fsm_lexer_eat_tkn(struct fsm_lexer* lexer) {
	++lexer->tkn_idx;
	_fsm_lexer_read_tkn(lexer);
	return 0;
}

fsm_err_t fsm_lexer_peek_tkn(struct fsm_lexer* lexer, struct fsm_lexer_tkn* tkn) {
	*tkn = lexer->tkn_list[lexer->tkn_idx];
	return 0;
}

fsm_err_t fsm_lexer_rewind(struct fsm_lexer* lexer) {
	--lexer->tkn_idx;
	return 0;
}

fsm_err_t fsm_lexer_rewind_n(struct fsm_lexer* lexer, size_t n) {
	if (n > lexer->tkn_idx) {
		fprintf(stderr, "Error: fsm_lexer_rewind_n()\n");
		return -ERR_PARSE;
	}
	lexer->tkn_idx -= n;
	return 0;
}
