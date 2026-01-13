#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define RED 0xFF0000
#define GRN 0x00FF00
#define BLU 0x0000FF
#define WHT 0xFFFFFF
#define BLK 0x000000

typedef struct {
	uint32_t x;
	uint32_t y;
	uint32_t r;
	uint32_t m;
	double fc;
	double dx;
	double dy;
} ball;

ball* create_ball(uint32_t x, uint32_t y, uint32_t r) {
	ball* b = (ball *)malloc(sizeof(ball));
	b->x = x;
	b->y = y;
	b->r = r;
	b->m = 10;
	b->fc = 0.05f;
	b->dx = 0.0f;
	b->dy = 0.0f;
	return b;
}

typedef struct {
	ball* balls;
	uint32_t size;
	uint32_t cap;
} ball_list;

ball_list* init_ball_list(uint32_t capacity) {
	ball_list* bl = (ball_list *)malloc(sizeof(ball_list));
	bl->balls = (ball *)malloc(sizeof(ball) * capacity);
	bl->size = 0;
	bl->cap = capacity;
	return bl;
}

void free_ball_list(ball_list* bl) {
	free(bl->balls);
	free(bl);
}

bool resize_ball_list(ball_list* bl) {
	printf("Attempting to realloc - size: %d\n",(int)(sizeof(ball) * bl->cap) * 2);
	ball* ptr = (ball *)realloc(bl->balls, sizeof(ball) * bl->cap * 2);
	if (ptr == NULL) {
		return false;
	}
	bl->balls = ptr;
	bl->cap *= 2;
	printf("SUCCESS\n");
	return true;
}

void append_ball(ball_list* bl, ball* b) {
	if (bl->size >= bl->cap) {
		bool suc = resize_ball_list(bl);
		if (!suc) {
			printf("FAILED APPEND\n");
			return;
		}
	}
	bl->balls[bl->size] = *b;
	bl->size++;
}

void whiten_screen(uint32_t* fbp, struct fb_var_screeninfo vinfo) {
	uint32_t num_pixels = vinfo.xres * vinfo.yres;
	for (int i = 0; i < num_pixels; i++) {
		fbp[i] = WHT;
	}
}

void draw_ball(ball* b, uint32_t* fbp, struct fb_var_screeninfo vinfo, uint32_t color) {
	uint32_t x1 = b->x, y1 = b->y, rc = b->r;
	uint32_t x0, yhi, ylo;
	for (x0 = x1 - rc; x0 < x1 + rc; x0++) {
		yhi = y1 + sqrt(rc*rc - (x0 - x1)*(x0 - x1));
		ylo = y1 - sqrt(rc*rc - (x0 - x1)*(x0 - x1)); 
		if ((yhi > vinfo.yres || ylo < 0) ||
		   (x0 > vinfo.xres || x0 < 0)) {
			continue;
		}
		for (uint32_t i = ylo; i < yhi; i++) {
			if (i > vinfo.yres || i < 0) {
				continue;
			}
			fbp[i * vinfo.xres + x0] = color;
		}
	}
}

void draw_balls(ball_list* bl, uint32_t* fbp, struct fb_var_screeninfo vinfo) {
	whiten_screen(fbp, vinfo);
	for (uint32_t i = 0; i < bl->size; i++) {
		draw_ball(&bl->balls[i], fbp, vinfo, BLU);
	}
}

void collide(ball* b1, ball* b2) {
	double dx = b2->x - b1->x;
	double dy = b2->y - b1->y;
	double dist = sqrt(dx*dx + dy*dy);
	double nx = dx / dist;
	double ny = dy / dist;

	double dvx = b2->dx - b1->dx;
	double dvy = b2->dy - b1->dy;
	double vel_along_normal = dvx*nx + dvy*ny;

	if (vel_along_normal > 0) {
		return;
	}

	double impulse = (2 * vel_along_normal) / (b1->m + b2->m);

	b1->dx += impulse * b2->m * nx;
	b1->dy += impulse * b2->m * ny;
	b2->dx -= impulse * b1->m * nx;
	b2->dy -= impulse * b1->m * ny;
}

void update_ball_collisions(ball_list* bl) {
	ball* tmp;
	ball* tmp2;
	for (uint32_t i = 0; i < bl->size; i++) {
		tmp = &bl->balls[i];
		for (uint32_t k = 0; k < bl->size; k++) {
			if (k == i) {
				continue;
			}
			tmp2 = &bl->balls[k];
			if (((tmp->x - tmp->r < tmp2->x + tmp2->r) ||
			   (tmp->x + tmp->r > tmp2->x - tmp2->r)) && 
		   	   ((tmp->y - tmp->r < tmp2->y + tmp2->r) ||
			   (tmp->y + tmp->r > tmp2->y - tmp2->r))) {
				collide(tmp, tmp2);
			}
		}
	}
}

void update_balls(ball_list* bl, struct fb_var_screeninfo vinfo, double step) {
	ball* tmp;
	for (uint32_t i = 0; i < bl->size; i++) {
		tmp = &bl->balls[i];
		if (tmp->y > vinfo.yres) {
		       tmp->dy *= -(1 - tmp->fc);
		} else {
			tmp->dy += 980 * step;
		}
		if (tmp->x > vinfo.xres || tmp->x < 0) {
			tmp->dx *= -(1 - tmp->fc);
		} else {
			tmp->dx += 15 * step;
		}	
		// update_ball_collisions(bl); // not working currently
		tmp->y += tmp->dy * step;
		tmp->x += tmp->dx * step;
	}	
}

int main(int argc, char** argv) {
	char* err_msg;
	bool b_err = false;

	ball_list* bl = init_ball_list(5);
	ball* tmp;
	int tmp_x = 100;
	for (int i = 0; i < 2; i++) {
		tmp = create_ball(tmp_x, 80, 50);
		tmp_x += 100;
		append_ball(bl, tmp);
	}
	printf("bl size: %d, bl capacity: %d\n", bl->size, bl->cap);	

	int fb = open("/dev/fb0", O_RDWR);
	if (fb < 0) {
		err_msg = "Failed to open: /dev/fb0";
		b_err = true;
		goto cleanup;	
	}
	struct fb_var_screeninfo vinfo;
	ioctl(fb, FBIOGET_VSCREENINFO, &vinfo);

	uint32_t screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	uint32_t* fbp = (uint32_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if ((int64_t)fbp == -1) {
		err_msg = "Failed mmap fbd";
		b_err = true;
		goto cleanup;
	}
	draw_balls(bl,fbp, vinfo); 	

	double step = 0.1f;
	bool done = false;
	
	struct timespec start, end, truestart;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	clock_gettime(CLOCK_MONOTONIC_RAW, &truestart);
	
	while (!done) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		if ((double)((end.tv_sec * 1e9 + end.tv_nsec)
		   - (start.tv_sec * 1e9 + start.tv_nsec))
		   >= step*1e9) {
			update_balls(bl, vinfo, step);
			draw_balls(bl, fbp, vinfo);
			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
		}
		if ((double)(end.tv_nsec - truestart.tv_nsec) >= 1e10) {
			done = true;
		}
	}

	cleanup:
		free_ball_list(bl);
		munmap(fbp, screensize);
		close(fb);
		if (b_err) {
			printf(err_msg);
		}
		return b_err;	
}
