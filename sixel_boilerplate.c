#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SVG_NANOSVG
	#include <nanosvg.h>
	#include <nanosvgrast.h>

	#define STB_IMAGE_WRITE_IMPLEMENTATION
	#include "stb_image_write.h"
#else
	#include <librsvg/rsvg.h>
	#include <cairo.h>

	#include <png.h>
#endif

#include <sixel.h>

int print_png_file(const char * file)
{
	int signaled = 0;
	SIXELSTATUS status = SIXEL_FALSE;
	sixel_encoder_t *encoder = NULL;
	status = sixel_encoder_new(&encoder, NULL);
	if (SIXEL_FAILED(status))
		return 1;
	status = sixel_encoder_set_cancel_flag(encoder, &signaled);
	if (SIXEL_FAILED(status))
		return 1;
	status = sixel_encoder_encode(encoder, file);
	if (SIXEL_FAILED(status)) {
		printf("failed to encode image to sixel\n");
		return 1;
	}
	printf("\n");
	sixel_encoder_unref(encoder);
	return 0;
}

int print_svg(char * image, size_t image_size, size_t width, size_t height)
{
#if defined(SVG_LIBRSVG)
	GError * error = NULL;
	RsvgHandle * handle = rsvg_handle_new_from_data((unsigned char *) image, image_size, &error);
	if (error) {
		printf("Error initilizing rsvg handle: %s\n", error->message);
		printf("svg: %s\n", image);
		g_error_free(error);
		return 1;
	}

	cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t * cr = cairo_create(surface);
	cairo_save(cr);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	RsvgRectangle vp = {
		.x = 0,
		.y = 0,
		.width = width,
		.height = height,
	};
	rsvg_handle_render_document(handle, cr, &vp, NULL);

	unsigned char * data = cairo_image_surface_get_data(surface);

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		goto svg_fail;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!png_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		goto svg_fail;
	}
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_bytepp row_pointers = (png_bytepp)malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++) {
		row_pointers[y] = (png_bytep)(data + y * width * 4);
	}
	for (int i = 0; i < width * height; ++i) {
		int a = (i * 4) + 3;
		if (data[a] < 0xff) {
			data[a] = 0xff;
			data[a - 1] = 0xff;
			data[a - 2] = 0xff;
			data[a - 3] = 0xff;
		} else {
			unsigned char tmp = data[a - 1];
			data[a - 1] = data[a - 3];
			data[a - 3] = tmp;
		}
	}
	char sixel_path[] = "/tmp/sixel_XXXXXX";
	int fd = mkstemp(sixel_path);
	if (fd == -1) {
		printf("failed to create tempfile %s\n", sixel_path);
		return 1;
	}
	FILE * fp = fdopen(fd, "wb");
	if (!fp)
		return 1;
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	fclose(fp);
	close(fd);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(row_pointers);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	g_object_unref(handle);

	return print_png_file(sixel_path);

	svg_fail:
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	g_object_unref(handle);
	return 1;
#elif defined(SVG_NANOSVG)
	NSVGimage * nsvg_image = nsvgParse(image, "px", 300.0f);
	if (!nsvg_image)
		goto svg_fail;

	NSVGrasterizer * nsvg_rast = nsvgCreateRasterizer();
	if (!nsvg_rast)
		goto svg_fail;

	size_t data_size = width * height * 4;
	unsigned char * data = malloc(data_size);
	float zoom = (float) width / nsvg_image->width;
	nsvgRasterize(nsvg_rast, nsvg_image, 0, (height / 2) - ((nsvg_image->height * zoom) / 2), zoom, data, width, height, width * 4);
	for (int i = 0; i < width * height; ++i) {
		int alphaIndex = (i * 4) + 3;
		if (data[alphaIndex] < 0xff) {
			data[alphaIndex] = 0xff;
			data[alphaIndex - 1] = 0xff;
			data[alphaIndex - 2] = 0xff;
			data[alphaIndex - 3] = 0xff;
		}
	}
	printf("%d-%d-%d-%d", data[0], data[1], data[2], data[3]);

	char sixel_path[] = "/tmp/sixel_XXXXXX";
	int fd = mkstemp(sixel_path);
	if (fd == -1) {
		printf("failed to create tempfile %s\n", sixel_path);
		return 1;
	}
	close(fd);

	stbi_write_png(sixel_path, width, height, 4, data, width * 4);

	nsvgDeleteRasterizer(nsvg_rast);
	nsvgDelete(nsvg_image);

	return print_png_file(sixel_path);

	svg_fail:
	nsvgDeleteRasterizer(nsvg_rast);
	nsvgDelete(nsvg_image);

	return 1;
#else
	return 0;
#endif
}

int print_png(unsigned char * image, size_t image_size, size_t width, size_t height)
{
	char sixel_path[] = "/tmp/sixel_XXXXXX";
	int fd = mkstemp(sixel_path);
	if (fd == -1) {
		printf("failed to create tempfile %s\n", sixel_path);
		return 1;
	}
	if (write(fd, image, image_size) == -1) {
		close(fd);
		remove(sixel_path);
		printf("failed to write to tempfile\n");
		return 1;
	}
	close(fd);
	int ret = print_png_file(sixel_path);
	remove(sixel_path);
	return ret;
}
