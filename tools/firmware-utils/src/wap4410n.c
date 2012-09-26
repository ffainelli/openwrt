/*
 * Cisco/Linksys WAP4410N image creator
 *
 * Copyright (C) 2012, Florian Fainelli <florian@openwrt.org>
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/md5.h>

/* Upgrade type is a bitmask, you can combine several upgrades */
#define UPGRADE_TYPE_AUTO	0x00
#define UPGRADE_TYPE_KERNEL	0x01
#define UPGRADE_TYPE_ROOTFS	0x02
#define UPGRADE_TYPE_BOOTROM	0x10
#define UPGRADE_TYPE_BOARD_DATA	0x20

#define PID_SIZE		112

struct input_file {
	const char *name;
	uint8_t type;
};

/* valid upgrade types */
static const struct input_file upgrade_types[] = {
	{ "auto", UPGRADE_TYPE_AUTO },
	{ "kernel", UPGRADE_TYPE_KERNEL },
	{ "rootfs", UPGRADE_TYPE_ROOTFS },
	{ "bootrom", UPGRADE_TYPE_BOOTROM },
	{ "board", UPGRADE_TYPE_BOARD_DATA },
};

#define ARRAY_SIZE(x)	(sizeof((x)) / sizeof((x[0])))

/* Main header */
struct lap_hdr {
	uint32_t	length;
	uint8_t		upgrade_type;
} __attribute__ ((__packed__));

/* File descriptors headers */
struct lap_desc_hdr {
	uint8_t upgrade_type;
	uint16_t version;
	uint32_t length;
} __attribute__ ((__packed__));

static void usage(void)
{
	fprintf(stdout, "Usage: mkwap4410n [options]\n"
			"-u:	upgrade type (auto, kernel, rootfs, bootrom...)\n"
			"-p:	PID file\n"
			"-b:	bootrom file\n"
			"-k:	kernel file\n"
			"-r:	roots file\n"
			"-d:	board data file\n"
			"-o:	output file\n");
	exit(EXIT_FAILURE);
}

static uint8_t parse_upgrade_type(const char *upgrade_type)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(upgrade_types); i++) {
		if (!strcmp(upgrade_types[i].name, upgrade_type))
			return upgrade_types[i].type;
	}

	return 0xff;
}

static off_t get_file_size(const char *file)
{
	struct stat buf;
	int ret;

	ret = stat(file, &buf);
	if (ret < 0)
		return 0;
	else
		return buf.st_size;
}

#define MAX_FILES	5

static int create_file(const char *upgrade_type, const char *version,
			struct input_file *infiles, unsigned int num_infiles,
			const char *outfile)
{
	int ret = 1;
	char *buf;
	FILE *fp;
	struct lap_hdr *lap_hdr;
	struct lap_desc_hdr *desc_hdr;
	size_t offset = 0;
	off_t file_size = 0;
	MD5_CTX md5;
	unsigned char digest[MD5_DIGEST_LENGTH];
	unsigned int i;
	uint16_t hex_version = 0;
	uint8_t hex_upgrade_type = 0;

	/* Parse version */
	ret = sscanf(version, "%04hx", &hex_version);
	if (ret < 0) {
		fprintf(stderr, "failed to parse version\n");
		return 1;
	}

	/* Parse upgrade type */
	hex_upgrade_type = parse_upgrade_type(upgrade_type);
	if (hex_upgrade_type > UPGRADE_TYPE_BOARD_DATA) {
		fprintf(stderr, "invalid upgrade type: %s\n", upgrade_type);
		return 1;
	}

	memset(&md5, 0, sizeof(md5));
	MD5_Init(&md5);

	/* Start writing the PID file */
	offset = PID_SIZE + sizeof(struct lap_hdr);
	buf = malloc(offset);
	if (!buf) {
		perror("malloc");
		return 1;
	}

	fp = fopen(infiles[0].name, "rb");
	if (!fp) {
		perror("fopen");
		goto out;
	}

	fread(buf, PID_SIZE, 1, fp);
	fclose(fp);

	/* prepare descriptors, PID file does not have one */
	for (i = 1; i < num_infiles; i++) {
		file_size = get_file_size(infiles[i].name);

		/*
 		 FIXME: check whether we just need to skip files or if we
		 should append an empty descriptor for them anyway
		if (!file_size)
			continue;
		*/

		fp = fopen(infiles[i].name, "rb");
		if (!fp) {
			perror("fopen");
			goto out;
		}

		/* reallocate as many memory as we need to append this file
		 * and its descriptor to the existing buffer
		 */
		buf = realloc(buf, offset + sizeof(struct lap_desc_hdr) + file_size);
		if (!buf) {
			perror("malloc");
			goto out;
		}

		desc_hdr = (struct lap_desc_hdr *)(buf + offset);
		desc_hdr->upgrade_type = infiles[i].type;
		desc_hdr->length = htobe32(file_size);

		/* version is valid only for bootrom or board data */
		if (infiles[i].type & (UPGRADE_TYPE_BOOTROM | UPGRADE_TYPE_BOARD_DATA))
			desc_hdr->version = htobe16(hex_version);

		offset += sizeof(struct lap_desc_hdr);

		/* read file into buffer at appropriate location */
		fread(buf + offset, file_size, 1, fp);
		fclose(fp);
		offset += file_size;
	}

	/* Now build the main header */
	lap_hdr = (struct lap_hdr *)(buf + PID_SIZE);
	lap_hdr->upgrade_type = hex_upgrade_type;
	lap_hdr->length = htobe32(offset);

	/* Compute MD5Sum on the entire file */
	MD5_Update(&md5, buf, offset);
	MD5_Final(digest, &md5);

	fp = fopen(outfile, "wb+");
	if (!fp) {
		perror("fopen");
		goto out;
	}

	/* Write down the buffer */
	fwrite(buf, offset, 1, fp);
	fflush(fp);
	/* And append the MD5 digest */
	fwrite(digest, sizeof(digest), 1, fp);
	fclose(fp);

	ret = 0;

out:
	free(buf);
	return ret;
}

int main(int argc, char **argv)
{
	int opt;
	const char *upgrade_type = NULL;
	const char *outfile = NULL;
	const char *version = NULL;
	struct input_file infiles[5];
	unsigned int index = 0;

	memset(&infiles, 0, sizeof(infiles));

	while ((opt = getopt(argc, argv, "u:p:b:k:r:d:v:o:h")) > 0) {
		switch (opt) {
		case 'u':
			upgrade_type = optarg;
			break;

		case 'p':
			infiles[index].type = ~0;
			infiles[index].name = optarg;
			index++;
			break;

		case 'b':
			infiles[index].type = UPGRADE_TYPE_BOOTROM;
			infiles[index].name = optarg;
			index++;
			break;

		case 'k':
			infiles[index].type = UPGRADE_TYPE_KERNEL;
			infiles[index].name = optarg;
			index++;
			break;

		case 'r':
			infiles[index].type = UPGRADE_TYPE_ROOTFS;
			infiles[index].name = optarg;
			index++;
			break;

		case 'd':
			infiles[index].type = UPGRADE_TYPE_BOARD_DATA;
			infiles[index].name = optarg;
			index++;
			break;

		case 'v':
			version = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	if (index < 2) {
		fprintf(stderr, "at least two input files are required\n");
		return 1;
	} else if (index > MAX_FILES) {
		fprintf(stderr, "too many files specified\n");
		return 1;
	}

	return create_file(upgrade_type, version, infiles, index, outfile);
}
