/*
 * Regression test for segment size bounds checking in elksemu.
 * Verifies that:
 *   - A properly formatted ELKS binary is accepted by load_elks()
 *   - A binary with tseg > 0x10000 is rejected
 *   - A binary with esh_ftseg > 0x10000 is rejected
 *
 * Usage: ./test_segment_bounds [path_to_elksemu]
 *        Default elksemu path is ./elksemu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "elks.h"

static const char *elksemu_path = "./elksemu";

static int run_elksemu(const char *binary_path, int *was_rejected)
{
	int pipefd[2];
	if (pipe(pipefd) < 0) {
		perror("pipe");
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}

	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		close(STDOUT_FILENO);
		execl(elksemu_path, "elksemu", binary_path, NULL);
		_exit(127);
	}

	close(pipefd[1]);
	char buf[512] = {0};
	read(pipefd[0], buf, sizeof(buf) - 1);
	close(pipefd[0]);

	int status;
	waitpid(pid, &status, 0);

	*was_rejected = (strstr(buf, "Unsupported") != NULL ||
			 strstr(buf, "Invalid argument") != NULL);

	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

static int create_valid_binary(const char *path)
{
	struct minix_exec_hdr mh;
	char code[64];

	memset(&mh, 0, sizeof(mh));
	mh.type = ELKS_SPLITID;
	mh.hlen = EXEC_MINIX_HDR_SIZE;
	mh.version = 1;
	mh.tseg = 64;
	mh.dseg = 0;
	mh.bseg = 0;
	mh.minstack = 0;
	mh.chmem = 0;

	memset(code, 0xF4, sizeof(code)); /* HLT instructions */

	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;
	fwrite(&mh, sizeof(mh), 1, f);
	fwrite(code, mh.tseg, 1, f);
	fclose(f);
	return 0;
}

static int create_oversized_tseg(const char *path)
{
	struct minix_exec_hdr mh;

	memset(&mh, 0, sizeof(mh));
	mh.type = ELKS_SPLITID;
	mh.hlen = EXEC_MINIX_HDR_SIZE;
	mh.version = 1;
	mh.tseg = 0x10001;
	mh.dseg = 0;
	mh.bseg = 0;

	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;
	fwrite(&mh, sizeof(mh), 1, f);
	fclose(f);
	return 0;
}

static int create_oversized_ftseg(const char *path)
{
	struct minix_exec_hdr mh;
	struct elks_supl_hdr esuph;

	memset(&mh, 0, sizeof(mh));
	memset(&esuph, 0, sizeof(esuph));
	mh.type = ELKS_SPLITID;
	mh.hlen = EXEC_FARTEXT_HDR_SIZE;
	mh.version = 1;
	mh.tseg = 64;
	mh.dseg = 0;
	mh.bseg = 0;
	esuph.esh_ftseg = 0x10001;

	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;
	fwrite(&mh, sizeof(mh), 1, f);
	fwrite(&esuph, sizeof(esuph), 1, f);
	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{
	int pass = 0, fail = 0;
	int rc, rejected;
	const char *tmp_valid = "/tmp/elks_test_valid.bin";
	const char *tmp_tseg = "/tmp/elks_test_tseg.bin";
	const char *tmp_ftseg = "/tmp/elks_test_ftseg.bin";

	if (argc > 1)
		elksemu_path = argv[1];

	if (access(elksemu_path, X_OK) != 0) {
		fprintf(stderr, "Cannot find elksemu at '%s'\n", elksemu_path);
		return 77; /* skip */
	}

	/* Test 1: valid binary should be accepted (not rejected by load_elks) */
	if (create_valid_binary(tmp_valid) < 0) {
		perror("create_valid_binary");
		return 1;
	}
	rc = run_elksemu(tmp_valid, &rejected);
	if (!rejected) {
		printf("PASS: valid binary accepted by load_elks()\n");
		pass++;
	} else {
		printf("FAIL: valid binary was rejected (rc=%d)\n", rc);
		fail++;
	}

	/* Test 2: oversized tseg should be rejected */
	if (create_oversized_tseg(tmp_tseg) < 0) {
		perror("create_oversized_tseg");
		return 1;
	}
	rc = run_elksemu(tmp_tseg, &rejected);
	if (rejected) {
		printf("PASS: oversized tseg rejected\n");
		pass++;
	} else {
		printf("FAIL: oversized tseg not rejected (rc=%d)\n", rc);
		fail++;
	}

	/* Test 3: oversized esh_ftseg should be rejected */
	if (create_oversized_ftseg(tmp_ftseg) < 0) {
		perror("create_oversized_ftseg");
		return 1;
	}
	rc = run_elksemu(tmp_ftseg, &rejected);
	if (rejected) {
		printf("PASS: oversized esh_ftseg rejected\n");
		pass++;
	} else {
		printf("FAIL: oversized esh_ftseg not rejected (rc=%d)\n", rc);
		fail++;
	}

	unlink(tmp_valid);
	unlink(tmp_tseg);
	unlink(tmp_ftseg);

	printf("\nResults: %d passed, %d failed\n", pass, fail);
	return fail ? 1 : 0;
}
