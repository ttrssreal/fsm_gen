#include "fsm.h"
#include <cstdio>

int main(int argc, char** argv) {
	struct fsm fsm;

	fprintf(stderr, "Example Program.\n");

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <spec_file>\n", argv[0]);
		return -1;
	}

	char* spec_file = argv[1];

	fsm_init(&fsm);

	fsm_err_t res = fsm_parse_spec(&fsm, spec_file);
	if (res < 0) {
		fprintf(stderr, "Error parsing spec\n");
		return res;
	}

	char* code;
	fsm_codegen(&fsm, code);

	printf("%s\n", code);

	fsm_release(&fsm);
	return 0;
}
