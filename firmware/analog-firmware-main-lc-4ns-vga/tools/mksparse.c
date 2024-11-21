#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#define CRC32_INIT                  ((uint32_t)-1l)
#define BLOCK_SIZE 512
#define FLASH_SIZE (256*1024)

uint8_t ota_buffer[FLASH_SIZE];
uint8_t block_list[FLASH_SIZE/BLOCK_SIZE];

static uint32_t soft_crc32_block(uint32_t crc, uint8_t *bytp, uint32_t length) {
    while(length--) {
        uint32_t byte32 = (uint32_t)*bytp++;

        for (uint8_t bit = 8; bit; bit--, byte32 >>= 1) {
            crc = (crc >> 1) ^ (((crc ^ byte32) & 1ul) ? 0xEDB88320ul : 0ul);
        }
    }
    return crc;
}

int is_solid_block(uint8_t *buffer, uint8_t ch, uint32_t size) {
	uint32_t i;

	for(i = 0; i < size; i++) {
		if(buffer[i] != ch) return 0;
	}

	return 1;
}

int main(int argc, char **argv) {
	uint32_t crc_result;
	FILE *in, *out;
	int i;

	memset(ota_buffer, 0xff, FLASH_SIZE);

	if(argc != 3) {
		fprintf(stderr, "Usage:\r\n\t%s <input> <output>\r\n", argv[0]);
		return -1;
	}

	in = fopen(argv[1], "rb");
	if(!in) {
		fprintf(stderr, "Unable to open input file '%s'.\r\n", argv[1]);
		return -1;
	}

	out = fopen(argv[2], "wb");
	if(!out) {
		fprintf(stderr, "Unable to open output file '%s'.\r\n", argv[2]);
		return -1;
	}

	fread(ota_buffer, 1, FLASH_SIZE, in);
	fclose(in);

	crc_result = soft_crc32_block(CRC32_INIT, ota_buffer, FLASH_SIZE-sizeof(uint32_t));
	ota_buffer[FLASH_SIZE - 4] = (crc_result >> 0) & 0xff;
	ota_buffer[FLASH_SIZE - 3] = (crc_result >> 8) & 0xff;
	ota_buffer[FLASH_SIZE - 2] = (crc_result >> 16) & 0xff;
	ota_buffer[FLASH_SIZE - 1] = (crc_result >> 24) & 0xff;

	for(i = 0; i < (FLASH_SIZE/BLOCK_SIZE); i++) {
		block_list[i] = (is_solid_block(ota_buffer+(i*BLOCK_SIZE), 0x00, BLOCK_SIZE) ? 1 : 0) |
				(is_solid_block(ota_buffer+(i*BLOCK_SIZE), 0xff, BLOCK_SIZE) ? 2 : 0);
	}

	fwrite(block_list, 1, (FLASH_SIZE/BLOCK_SIZE), out);

	for(i = 0; i < (FLASH_SIZE/BLOCK_SIZE); i++) {
		if(!block_list[i])
			fwrite(ota_buffer+(i*BLOCK_SIZE), 1, BLOCK_SIZE, out);
	}

	fclose(out);
	return 0;
}
