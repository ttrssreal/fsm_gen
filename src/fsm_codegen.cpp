#include "fsm.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

fsm_err_t read_def_file(int fd_def, char*& out) {
	size_t buff_cap = 1024;
	size_t buff_len = 0;

	out = (char*)malloc(buff_cap);
	if (!out) {
		fprintf(stderr, "Error: read_def_file()\n");
		return -ERR_ALLOC;
	}
	
	ssize_t bytes_read;
	while ((bytes_read = read(fd_def, out + buff_len, 1)) != 0) {
		if (bytes_read == -1) {
			fprintf(stderr, "Error: read_def_file()\n");
			return -ERR_ALLOC;
		}
		buff_len += bytes_read;
		if (buff_len + 1 >= buff_cap) {
			buff_cap *= 2;
			out = (char*)realloc(out, buff_cap);
			if (!out) {
				fprintf(stderr, "Error: read_def_file()\n");
				return -ERR_ALLOC;
			}
		}
	}

	*(out + buff_len) = 0;
	return 0;
}

fsm_err_t fsm_codegen_gen_state(struct fsm* fsm, struct fsm_st* st, char*& out) {
	strcat(out, "if ( state==\"");
	strcat(out, st->name);
	strcat(out, "\"){  \n");
	for (size_t tr_idx = 0; tr_idx < st->tr_count; ++tr_idx) {
		struct fsm_tr* tr = &st->tr_list[tr_idx];
		strcat(out, "   if ( input==\"");
		strcat(out, tr->inp);
		strcat(out, "\"){\n");
		strcat(out, "     state=\"");
		strcat(out,  RESLV_STATE(fsm->st_list, tr->st_dst).name);
		strcat(out, "\"; \n");
		if (tr->st_dst == fsm->st_acc) {
			strcat(out, "     return 1;\n");
		}
		strcat(out, "   }\n");
	}
	strcat(out, "}\n");
	return 0;
}

fsm_err_t fsm_codegen(struct fsm* fsm, char*& generated) {

	int fd_prolouge;
	int fd_epilouge;

	if (!(fd_prolouge = open(_CODEGEN_PROLOUGE_FNAME, O_RDONLY))) {
		fprintf(stderr, "Error: opening file " _CODEGEN_PROLOUGE_FNAME "\n");
		return -ERR_FILE_OPEN;
	}

	if (!(fd_epilouge = open(_CODEGEN_EPILOUGE_FNAME, O_RDONLY))) {
		fprintf(stderr, "Error: opening file " _CODEGEN_EPILOUGE_FNAME "\n");
		return -ERR_FILE_OPEN;
	}

	char* prolouge;
	fsm_err_t err = read_def_file(fd_prolouge, prolouge);
	if (err) {
		fprintf(stderr, "Error: fsm_codegen()\n");
		return err;
	}

	generated = prolouge;

	strcat(generated, "int Proc(string input)\n{\n");

	for (size_t st_idx = 0; st_idx < fsm->st_count; ++st_idx) {
		fsm_codegen_gen_state(fsm, &fsm->st_list[st_idx], generated);
		strcat(generated, "else\n");
	}

	strcat(generated, "{}\nreturn 0;\n}\n\n");

	strcat(generated, "int main(){\nstate=\"");
	strcat(generated, fsm->st_list[fsm->st_init].name);
	strcat(generated, "\";\n\n");

	char* epilouge;
	err = read_def_file(fd_epilouge, epilouge);
	if (err) {
		fprintf(stderr, "Error: fsm_codegen()\n");
		return err;
	}

	strcat(generated, epilouge);

	return 0;
}
