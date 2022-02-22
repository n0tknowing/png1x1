/*
 * 1x1 PNG image generator
 *
 * public domain
 * 
 * */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

static uint32_t endian_swap32(uint32_t n)
{
	uint8_t *i = (uint8_t *)&n;
	return ((i[3] << 0) | (i[2] << 8) | (i[1] << 16) | (i[0] << 24));
}

static void png_write_signature(FILE *f)
{
	const uint8_t buf[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
	if (fwrite(buf, 1u, 8u, f) != 8u) {
		fprintf(stderr, "failed to write: ");
		fprintf(stderr, "%s\n", feof(f) ? "EOF reached" : "I/O error");
	}
}

static void png_write_ihdr(FILE *f)
{
	const uint8_t buf[] = {
		0x00, 0x00, 0x00, 0x0d,
		0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x01,
		0x08, 0x02, 0x00, 0x00, 0x00,
		0x90, 0x77, 0x53, 0xde
	};

	if (fwrite(buf, 1u, 25u, f) != 25u) {
		fprintf(stderr, "failed to write: ");
		fprintf(stderr, "%s\n", feof(f) ? "EOF reached" : "I/O error");
	}
}

static void png_write_idat(FILE *f, const uint8_t *col)
{
	const uint8_t type[] = {0x49, 0x44, 0x41, 0x54};
	uint8_t data[] = {0x00, col[0], col[1], col[2]};

	Bytef out_buf[50];

	z_stream z = {
		.avail_in = 4,
		.next_in = data,
		.avail_out = 50,
		.next_out = out_buf
	};

	deflateInit(&z, Z_BEST_COMPRESSION);
	deflate(&z, Z_FINISH);
	deflateEnd(&z);

	const uint32_t length = endian_swap32(z.total_out);
	if (fwrite(&length, sizeof(uint32_t), 1, f) != 1) {
		fprintf(stderr, "idat write length failed\n");
		return;
	}

	if (fwrite(type, 1, 4, f) != 4) {
		fprintf(stderr, "idat write type failed\n");
		return;
	}

	if (fwrite(out_buf, 1, z.total_out, f) != z.total_out) {
		fprintf(stderr, "idat write compressed data failed\n");
		return;
	}

	uint32_t crc = crc32(0, (Bytef *)type, 4);
	crc = crc32(crc, (Bytef *)out_buf, z.total_out);
	crc = endian_swap32(crc);

	if (fwrite(&crc, sizeof(uint32_t), 1, f) != 1) {
		fprintf(stderr, "idat write crc failed\n");
		return;
	}
}

static void png_write_iend(FILE *f)
{
	const uint8_t buf[] = {
		0x00, 0x00, 0x00, 0x00,
		0x49, 0x45, 0x4e, 0x44,
		0xae, 0x42, 0x60, 0x82
	};

	if (fwrite(buf, 1u, 12u, f) != 12u) {
		fprintf(stderr, "failed to write: ");
		fprintf(stderr, "%s\n", feof(f) ? "EOF reached" : "I/O error");
	}
}

static void png_write_all(FILE *f, const uint8_t *color_data)
{
	png_write_signature(f);
	png_write_ihdr(f);
	png_write_idat(f, color_data);
	png_write_iend(f);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s #rrggbb out.png\n", argv[0]);
		fprintf(stderr, "\n");
		fprintf(stderr, "alpha channel is not supported, only RRGGBB\n");
		return 1;
	}

	errno = 0;

	const char *fname = argv[2];
	FILE *f = fopen(fname, "wb+");
	if (!f) {
		perror("cannot open file");
		return 1;
	}

	const char *rgb_input = argv[1];
	if (rgb_input[0] == '#')
		rgb_input += 1;
	else if (!strncmp(rgb_input, "0x", 2))
		rgb_input += 2;

	uint8_t rgb[3] = {0};
	if (sscanf(rgb_input, "%2hhx%2hhx%2hhx", &rgb[0], &rgb[1], &rgb[2]) != 3)
		perror("sscanf failed");
	else
		png_write_all(f, rgb);

	fclose(f);
	return 0;
}
