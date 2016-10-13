#include <curses.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define DELAY 1
#define WIDTH 80
#define HEIGHT 25
#define MAXLEN ((WIDTH - 2) * (HEIGHT - 2) + 1)

#define GREET "VimSnake, by ckj, 2016"
#define GHINT "use 'hjkl' to move, 'q' to quit"
#define OVER "Game Over"
#define OHINT "press 'r' to restart, 'q' to quit"

#define BLANK ' '
#define SNAKE '@'
#define WALL '#'
#define FOOD '$'
#define BOOM '!'

enum {
	IDLE	= 0,
	UP	= 1,
	DOWN	= 2,
	LEFT	= 3,
	RIGHT	= 4,
};

struct point {
	int x;
	int y;
};

struct snake {
	int head;
	int tail;
	struct point p[MAXLEN];
};

struct snake sn;
struct point food;
int heading;
jmp_buf env;

void genfood()
{
	do {
		food.x = rand() % (HEIGHT - 2) + 1;
		food.y = rand() % (WIDTH - 2) + 1;
	} while (mvinch(food.x, food.y) == SNAKE);
	mvaddch(food.x, food.y, FOOD);
}

void reset()
{
	sn.head = 0;
	sn.tail = 0;
	sn.p[0] = (struct point){HEIGHT / 2, WIDTH / 2};
	heading = IDLE;
	for (int i = 0; i <= HEIGHT; i++) {
		mvaddch(i, 0, WALL);
		mvaddch(i, WIDTH, WALL);
	}
	for (int j = 0; j <= WIDTH; j++) {
		mvaddch(0, j, WALL);
		mvaddch(HEIGHT, j, WALL);
	}
	for (int i = 1; i < HEIGHT; i++)
		for (int j = 1; j < WIDTH; j++)
			mvaddch(i, j, BLANK);
	genfood();
	mvaddch(sn.p[0].x, sn.p[0].y, SNAKE);
	refresh();
}

void init()
{
	srand(time(0));
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	for (int i = 0; i < HEIGHT; i++) {
		mvaddch(i, 0, WALL);
		mvaddch(i, WIDTH, WALL);
	}
	for (int j = 0; j < WIDTH; j++) {
		mvaddch(0, j, WALL);
		mvaddch(HEIGHT, j, WALL);
	}
	mvaddstr(HEIGHT / 2, (WIDTH - sizeof(GREET) + 1) / 2, GREET);
	mvaddstr(HEIGHT / 2 + 1, (WIDTH - sizeof(GHINT) + 1) / 2, GHINT);
	refresh();
}

void quit(int code)
{
	endwin();
	exit(code);
}

void gameover()
{
	int c;

	mvaddstr(HEIGHT / 2, (WIDTH - sizeof(OVER) + 1) / 2, OVER);
	mvaddstr(HEIGHT / 2 + 1, (WIDTH - sizeof(OHINT) + 1) / 2, OHINT);
	refresh();
	while ((c = getch())) {
		if (c == 'q')
			quit(0);
		if (c == 'r')
			longjmp(env, 0);
	}
}

struct point forward(struct point orig)
{
	struct point next;

	switch(heading) {
	case LEFT:
		next.x = orig.x;
		next.y = orig.y - 1;
		break;
	case DOWN:
		next.x = orig.x + 1;
		next.y = orig.y;
		break;
	case UP:
		next.x = orig.x - 1;
		next.y = orig.y;
		break;
	case RIGHT:
		next.x = orig.x;
		next.y = orig.y + 1;
		break;
	default:
		return orig;
	}
	return next;
}

void control(int c)
{
	switch (c) {
	case 'h':
		heading = heading == RIGHT ? RIGHT : LEFT;
		break;
	case 'j':
		heading = heading == UP ? UP : DOWN;
		break;
	case 'k':
		heading = heading == DOWN ? DOWN : UP;
		break;
	case 'l':
		heading = heading == LEFT ? LEFT : RIGHT;
		break;
	case 'q':
		quit(0);
	default:
		return;
	}
}

int check(struct point p)
{
	if (!(p.x > 0 && p.x < HEIGHT && p.y > 0 && p.y < WIDTH))
		return -1;
	if (mvinch(p.x, p.y) == WALL || mvinch(p.x, p.y) == SNAKE)
		return -1;
	if (p.x == food.x && p.y == food.y)
		return 1;
	return 0;
}

void tock()
{
	int rv;

	sn.p[(sn.head + 1) % MAXLEN] = forward(sn.p[sn.head]);
	sn.head = (sn.head + 1) % MAXLEN;
	mvaddch(sn.p[sn.tail].x, sn.p[sn.tail].y, BLANK);
	if ((rv = check(sn.p[sn.head])) != 1) {
		sn.tail = (sn.tail + 1) % MAXLEN;
	} else {
		mvaddch(sn.p[sn.tail].x, sn.p[sn.tail].y, SNAKE);
		genfood();
	}
	if (rv == -1) {
		mvaddch(sn.p[sn.head].x, sn.p[sn.head].y, BOOM);
		gameover();
	}
	mvaddch(sn.p[sn.head].x, sn.p[sn.head].y, SNAKE);
	refresh();
}

void tick(int sig)
{
	signal(SIGALRM, tick);
	struct itimerval gaptime;

	gaptime.it_interval.tv_sec = 0;
	gaptime.it_interval.tv_usec = 200000;

	gaptime.it_value.tv_sec = 0;
	gaptime.it_value.tv_usec = 200000;

	setitimer(ITIMER_REAL, &gaptime, NULL);

	tock();
}

void run()
{
	int c;

	while ((c = getch())) {
		control(c);
	}
}

int main()
{
	signal(SIGALRM, tick);
	init();
	sleep(2);
	setjmp(env);
	reset();
	alarm(1);
	run();
	exit(0);
}
