#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sixel.h>

int print_sixel(unsigned char * image, size_t image_size, size_t width, size_t height)
{
	int signaled = 0;
	SIXELSTATUS status = SIXEL_FALSE;
	sixel_encoder_t *encoder = NULL;
	status = sixel_encoder_new(&encoder, NULL);
	if (SIXEL_FAILED(status)) {
		return 1;
	}
	status = sixel_encoder_set_cancel_flag(encoder, &signaled);
	if (SIXEL_FAILED(status)) {
		return 1;
	}
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
	status = sixel_encoder_encode(encoder, sixel_path);
	printf("\n");
	remove(sixel_path);
	if (SIXEL_FAILED(status)) {
		printf("failed to encode image to sixel\n");
		return 1;
	}
	sixel_encoder_unref(encoder);
	return 0;
}
