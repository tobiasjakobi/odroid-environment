#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

extern void nv12mt_to_yuv420m_neon(void *y_out,
	void *cb_out, void *cr_out, const void *y_in,
	const void *cbcr_in, unsigned width, unsigned height);

enum nv12mt_constants {
	nv12mt_tile_width = 64,
	nv12mt_tile_height = 32,
	nv12mt_tile_sz = nv12mt_tile_width * nv12mt_tile_height,
};

enum nv12_plane {
	nv12_luma = 0,
	nv12_chroma = 1,
};

static unsigned
tile_pos(size_t x, size_t y, size_t w, size_t h)
{
	unsigned pos = x + (y & ~1) * w;

	if (y & 1) {
		pos += (x & ~3) + 2;
	} else if ((h & 1) == 0 || y != (h - 1)) {
		pos += (x + 2) & ~3;
	}

	return pos;
}

/*
 * NV12 is bi-planar. input[nv12_luma] points to the luma plane,
 * while input[nv12_chroma] to the chroma plane. Same applies
 * to output[].
 * Based on gstreamer code.
 */
static int
convert_nv12_nv12mt(uint8_t* tiled[2], uint8_t* untiled[2],
					unsigned width, unsigned height, bool untile)
{
	if (width == 0 || height == 0)
		return -1;

	uint8_t *tiled_ptr, *untiled_ptr;
	uint8_t **src, **dst;

	if (untile) {
		src = &tiled_ptr;
		dst = &untiled_ptr;
	} else {
		src = &untiled_ptr;
		dst = &tiled_ptr;
	}

	const unsigned luma_stride = width;
	const unsigned chroma_stride = width;

	const unsigned tile_w = (width - 1) / nv12mt_tile_width + 1;
	const unsigned tile_w_align = (tile_w + 1) & ~1;
	const unsigned tile_h_luma = (height - 1) / nv12mt_tile_height + 1;
	const unsigned tile_h_chroma = (height / 2 - 1) / nv12mt_tile_height + 1;

	for (unsigned y = 0; y < tile_h_luma; y++) {
		unsigned row_width = width;

		for (unsigned x = 0; x < tile_w; x++) {
			unsigned tile_width = row_width;
			unsigned tile_height = height;
			unsigned luma_idx, chroma_idx;

			/* luma source pointer for this tile */
			uint8_t *tiled_luma = tiled[nv12_luma] + tile_pos(x, y,
				tile_w_align, tile_h_luma) * nv12mt_tile_sz;

			/* chroma source pointer for this tile */
			uint8_t *tiled_chroma = tiled[nv12_chroma] + tile_pos(x, y / 2,
				tile_w_align, tile_h_chroma) * nv12mt_tile_sz;
			if (y & 1)
				tiled_chroma += nv12mt_tile_sz / 2;

			/* account for right columns */
			if (tile_width > nv12mt_tile_width)
				tile_width = nv12mt_tile_width;

			/* account for bottom rows */
			if (tile_height > nv12mt_tile_height)
				tile_height = nv12mt_tile_height;

			/* luma memory index for this tile */
			luma_idx = y * nv12mt_tile_height * luma_stride + x * nv12mt_tile_width;

			/* chroma memory index for this tile */
			chroma_idx = y * nv12mt_tile_height / 2 * chroma_stride + x * nv12mt_tile_width;

			/* copy 2 luma lines at once */
			tile_height /= 2;
			while (tile_height--) {
				untiled_ptr = untiled[nv12_luma] + luma_idx;
				tiled_ptr = tiled_luma;
				memcpy(*dst, *src, tile_width);
				tiled_luma += nv12mt_tile_width;
				luma_idx += luma_stride;

				untiled_ptr = untiled[nv12_luma] + luma_idx;
				tiled_ptr = tiled_luma;
				memcpy(*dst, *src, tile_width);
				tiled_luma += nv12mt_tile_width;
				luma_idx += luma_stride;

				untiled_ptr = untiled[nv12_chroma] + chroma_idx;
				tiled_ptr = tiled_chroma;
				memcpy(*dst, *src, tile_width);
				tiled_chroma += nv12mt_tile_width;
				chroma_idx += chroma_stride;
			}
			row_width -= nv12mt_tile_width;
		}
		height -= nv12mt_tile_height;
	}

	return 0;
}

static int
compare_buffers(const uint8_t* buf1[2], const uint8_t* buf2[2],
				unsigned width, unsigned height, bool luma,
				bool chroma)
{
	const unsigned y_size = width * height;
	const unsigned cbcr_size = width * height / 2;

	if (!luma)
		goto check_chroma;

	for (unsigned i = 0; i < y_size; ++i) {
		if (buf1[nv12_luma][i] != buf2[nv12_luma][i]) {
			fprintf(stderr, "ERROR: luma mismatch at %u\n", i);
			return -1;
		}
	}

check_chroma:
	if (!chroma)
		goto out;

	for (unsigned i = 0; i < cbcr_size; ++i) {
		if (buf1[nv12_chroma][i] != buf2[nv12_chroma][i]) {
			fprintf(stderr, "ERROR: chroma mismatch at %u\n", i);
			return -2;
		}
	}

out:
	return 0;
}

static int
tile_untile_test()
{
	const unsigned width = 1280;
	const unsigned height = 768;

	// Y plane / luma: 1 byte per pixel
	const unsigned y_size = width * height;

	// CbCr plane / chroma: 1 byte per pixel, but half the height
	const unsigned cbcr_size = width * height / 2;

	int ret;

	uint8_t *input = malloc(y_size + cbcr_size);
	uint8_t *output[2] = {
		malloc(y_size + cbcr_size),
		malloc(y_size + cbcr_size)
	};

	srand(time(NULL));
	for (unsigned i = 0; i < y_size + cbcr_size; ++i)
		input[i] = random();

	// Convert from NV12MT to NV12 (untile)
	convert_nv12_nv12mt(
		(uint8_t*[2]){input, input + y_size},
		(uint8_t*[2]){output[0], output[0] + y_size},
		width, height, true);

	// Convert from NV12 to NV12MT (tile)
	convert_nv12_nv12mt(
		(uint8_t*[2]){output[1], output[1] + y_size},
		(uint8_t*[2]){output[0], output[0] + y_size},
        width, height, false);

	ret = compare_buffers(
		(const uint8_t*[2]){input, input + y_size},
		(const uint8_t*[2]){output[1], output[1] + y_size},
		width, height, true, true);

out:
	free(output[0]);
	free(output[1]);
	free(input);

	return ret;
}

static int neon_test()
{
	const unsigned width = 128;
	const unsigned height = 64;
	const unsigned y_size = width * height;
	const unsigned cbcr_size = width * height / 2;

	int ret;

	uint8_t *input = malloc(y_size + cbcr_size);
	uint8_t *output[2] = {
		malloc(y_size + cbcr_size),
		malloc(y_size + cbcr_size)
	};

	srand(time(NULL));
	for (unsigned i = 0; i < y_size + cbcr_size; ++i)
	input[i] = random();

	nv12mt_to_yuv420m_neon(
		output[0], output[0] + y_size,
		output[0] + y_size + cbcr_size / 2,
		input, input + y_size, width, height);

	// Convert from NV12MT to NV12 (untile)
	convert_nv12_nv12mt(
		(uint8_t*[2]){input, input + y_size},
		(uint8_t*[2]){output[1], output[1] + y_size},
		width, height, true);

	ret = compare_buffers(
		(const uint8_t*[2]){output[0], NULL},
		(const uint8_t*[2]){output[1], NULL},
		width, height, true, false);

	free(output[0]);
	free(output[1]);
	free(input);

	return ret;
}

int main(int argc, char* argv[])
{
	int ret;

	ret = tile_untile_test();
	if (ret == 0)
		ret = neon_test();

	return ret;
}
