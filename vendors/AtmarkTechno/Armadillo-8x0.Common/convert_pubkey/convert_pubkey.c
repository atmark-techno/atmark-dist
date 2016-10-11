#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <openssl/pem.h>
#include <openssl/bn.h>

#define DATA_OUT	stdout
#define OTHER_OUT	stderr

enum {
	OUT_E = 0,
	OUT_N,
	OUT_BASE64,
};

static void dump_hex(const unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((i % 15) == 0)
			fprintf(OTHER_OUT, "\n    ");
		fprintf(OTHER_OUT, "%02x:", buf[i]);
	}

	fprintf(OTHER_OUT, "\n");
}

static void usage(void)
{
	fprintf(OTHER_OUT, "convert_pubkey [-endh] infile\n"
		"\t-b output base64 decode data (default)\n"
		"\t-e output e element\n"
		"\t-n output n element\n"
		"\t-d debug mode\n"
		"\t-h display this help and exit\n");

	exit(1);
}

static int unbase64(unsigned char **data, FILE *infile)
{
	BIO *b64, *bmem;
	int filesize;
	int len = 0;
	unsigned char *buf;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new_fp(infile, BIO_NOCLOSE);
	bmem = BIO_push(b64, bmem);

	fseek(infile, 0, SEEK_END);
	filesize = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	buf = OPENSSL_malloc(EVP_DECODE_LENGTH(filesize));
	len = BIO_read(bmem, buf, EVP_DECODE_LENGTH(filesize));

	BIO_free_all(bmem);

	*data = buf;

	return len;
}

static int bn_print_mem(unsigned char **data, FILE *infile, int type)
{
	RSA *rsa;
	BIGNUM *a;
	int len;
	unsigned char *buf;

	rsa = PEM_read_RSA_PUBKEY(infile, NULL, NULL, NULL);
	if (rsa == NULL) {
		fprintf(OTHER_OUT, "Couldn't read public key file\n");
		return -1;
	}

	if (type == OUT_E)
		a = rsa->e;
	else
		a = rsa->n;

	len = BN_num_bytes(a);
	buf = OPENSSL_malloc(len);

	BN_bn2bin(a, buf);

	*data = buf;

	return len;
}

int main(int argc, char *argv[])
{
	FILE *infile;
	unsigned char *data;
	int len;
	int c;
	int output = OUT_BASE64;
	int debug = 0;
	int ret = EXIT_FAILURE;

	while (1) {
		c = getopt(argc, argv, "bendh");
		if (c == -1)
			break;
		switch (c) {
		case 'b':
			output = OUT_BASE64;
			break;
		case 'e':
			output = OUT_E;
			break;
		case 'n':
			output = OUT_N;
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			usage();
		default:
			fprintf(OTHER_OUT, "Invalid option %c", optopt);
			usage();
		}
	}

	if (optind >= argc)
		usage();

	infile = fopen(argv[optind], "r");
	if (infile == NULL) {
		fprintf(OTHER_OUT, "Error opening file %s\n", argv[optind]);
		goto out;
	}

	switch (output) {
	case OUT_BASE64:
		len = unbase64(&data, infile);
		break;
	case OUT_E:
		len = bn_print_mem(&data, infile, OUT_E);
		break;
	case OUT_N:
		len = bn_print_mem(&data, infile, OUT_N);
		break;
	default:
		break;
	}

	if (len < 0)
		goto free_res;

	if (debug)
		dump_hex(data, len);
	else
		fwrite(data, 1, len, DATA_OUT);

	ret = EXIT_SUCCESS;

free_res:
	OPENSSL_free(data);
	fclose(infile);

out:
	return ret;
}
