/*
 * Copyright (c) 2015 Johannes Postma <jgmpostma@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/select.h>
#include <err.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "8tracks.h"
#include "curl.h"
#include "libplayer/player.h"

enum playcmd {
	NEXT,
	SKIP,
	SKIPMIX
};

extern char	*__progname;
static struct	termios termios;
static int	quitflag;

static int	nbgetchar(void);	/* non-blocking getchar */
static void	play(const char *, int);
static void	playmix(int, const char *);
static int	playtrack(int, struct track *, const char *);
static void	printshortmix(struct mix *);
static void	printtime(void);
static void	resettermios(void);
static void	search(const char *);
static int	settermios(void);
static void	signalhandler(int);
static void	usage(void);

/*
 * Non-blocking getchar
 * Returns the character if one is available, else it returns -1.
 */
static int
nbgetchar(void)
{
	struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
	fd_set fds;
	int nr;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	if (FD_ISSET(STDIN_FILENO, &fds))
		nr = getchar();
	else
		nr = -1;
	return nr;
}

static void
play(const char *url, int cflag)
{
	struct mix *mix = NULL;
	char *playtoken;
	int mixid;

	settermios();
	player_init();

	playtoken = getplaytoken();
	if (playtoken == NULL) {
		printf("Could not get a playtoken\n");
		goto end;
	}

	mix = mix_getbyurl(url);
	if (mix == NULL) {
		printf("Mix not found.\n");
		goto end;
	}
start:
	printf("%s by %s\n", mix->name, mix->user);
	playmix(mix->id, playtoken);

	if (cflag && !quitflag) {
		mixid = mix->id;
		mix_free(mix);
		mix = mix_getbysimilar(mixid, playtoken);
		if (mix == NULL)
			printf("Could not get the next mix.\n");
		else
			goto start;
	}
 end:
	mix_free(mix);
	free(playtoken);
	player_exit();
	resettermios();
}

static void
playmix(int mixid, const char *playtoken)
{
	struct track *track;
	int cmd, i;

	track = track_getfirst(mixid, playtoken);
	if (track == NULL) {
		printf("Could not load the playlist.\n");
		return;
	}
	for (i = 1; track != NULL; ++i) {
		printf("%02d. %s - %s\n", i, track->performer, track->name);
		cmd = playtrack(mixid, track, playtoken);
		track_free(track);
		if (quitflag)
			break;
		if (cmd == NEXT)
			track = track_getnext(mixid, playtoken);
		else if (cmd == SKIP)
			track = track_getskip(mixid, playtoken);
		else if (cmd == SKIPMIX)
			break;
	}
}

/*
 * Playtrack plays the track and checks for user input.  It returns a
 * suggestion on what to do next.  It can return NEXT to suggest that the track
 * has finished without user interruption and it is ready for the next track,
 * or it can return SKIP or SKIPMIX.
 */
static int
playtrack(int mixid, struct track *track, const char *playtoken)
{
	struct timespec tm = { .tv_sec = 0, .tv_nsec = 50000000 };
	int ch, cmd = NEXT, reportflag = 0;

	player_play(track->url);
	while (player_getstatus() != STOPPED) {
		if (quitflag) {
			player_stop();
			goto end;
		}
		printtime();
		ch = nbgetchar();
		switch (ch) {
		case 'q':
			printf("Quitting...\n");
			player_stop();
			quitflag = 1;
			goto end;
		case '\n':
		case '\r':
			printf("Skipping...\n");
			if (!track->skipallowedflag) {
				printf("Skip not allowed.\n");
			} else {
				player_stop();
				cmd = SKIP;
				goto end;
			}
			break;
		case '>':
			printf("Skipping mix...\n");
			player_stop();
			cmd = SKIPMIX;
			goto end;
		case 'p':
		case ' ':
			player_togglepause();
			break;
		default:
			break;
		}
		if (!reportflag && player_getposition() > 30) {
			report(track->id, mixid, playtoken);
			reportflag = 1;
		}
		nanosleep(&tm, NULL);
	}
end:
	return cmd;
}

static void
printshortmix(struct mix *mix)
{
	printf("%s\n", mix->url + 1);	/* skip first character */
	printf("\t%s\n", mix->tags);
	if (mix->certification != NULL)
		printf("\tcert: %s\n", mix->certification);
	if (mix->duration < 0) {
		printf("\tplays: %d\tlikes: %d\t(%d tracks)\n",
		    mix->playscount, mix->likescount, mix->trackscount);
	} else {
		printf("\tplays: %d\t likes: %d\t%d min (%d tracks)\n",
		    mix->playscount, mix->likescount, mix->duration / 60,
		    mix->trackscount);
	}
}

static void
query(const char *url)
{
	struct mix *mix;

	mix = mix_getbyurl(url);
	if (mix == NULL) {
		printf("Mix not found.\n");
		goto end;
	}
	printf("Mix name:\t%s ", mix->name);
	printf("(id: %d)\n", mix->id);
	printf("Created by:\t%s ", mix->user);
	printf("(id: %d)\n", mix->userid);
	printf("Description:\n%s\n", mix->description);
	printf("Tags:\t%s\n", mix->tags);
	if (mix->certification)
		printf("Certification:\t%s\n", mix->certification);
	printf("Plays: %d\t likes: %d\t%d min (%d tracks)\n",
	    mix->playscount, mix->likescount, mix->duration / 60,
	    mix->trackscount);
end:
	mix_free(mix);
}

static void
printtime(void)
{
	int duration, position;

	if (player_getstatus() == PAUSED) {
		printf("(paused)       \r");
	} else {
		duration = player_getduration();
		position = player_getposition();
		printf("%02d:%02d/%02d:%02d\r",
		    position / 60, position % 60,
		    duration / 60, duration % 60);
	}
	fflush(stdout);
}

static void
resettermios(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);
}

static void
search(const char *smartid)
{
	struct mix **mix;
	size_t len;
	int i;

	mix = mixset_searchbysmartid(smartid, &len);
	if (mix == NULL) {
		printf("Search returned no results.\n");
		return;
	}
	for (i = 0; (size_t) i < len; ++i)
		printshortmix(mix[i]);
	mixset_free(&mix, len);
}

/*
 * Disables echo and canonical mode, so we can register key hits without
 * them being displayed.
 */
static int
settermios(void)
{
	struct termios buf;

	if (tcgetattr(STDIN_FILENO, &termios))
		goto error;

	buf = termios;
	buf.c_lflag &= ~(ECHO | ICANON);
	buf.c_cc[VMIN] = 1;	/* read per byte */

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &buf) < 0)
		goto error;
	return 0;
error:
	return -1;
}

static void
signalhandler(int n)
{
	if (n == SIGINT)
		quitflag = 1;
}

static void
usage(void)
{
	fprintf(stderr, "usage %s:\n"
	    "\t%s [-P [-c]] URL\t\tPlay\n"
	    "\t%s -S SmartID\t\tSearch\n"
	    "\t%s -Q URL\t\t\tDisplay mix info\n",
	    __progname, __progname, __progname, __progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int cflag = 0, ch;
	enum {
		PLAY,
		SEARCH,
		QUERY
	} cmd = PLAY;

	quitflag = 0;
	setlocale(LC_ALL, "");
	signal(SIGINT, signalhandler);

	while ((ch = getopt(argc, argv, "PcSQ")) != -1) {
		switch (ch) {
		default:
		case 'P':
			cmd = PLAY;
			break;
		case 'c':
			if (cmd != PLAY)
				usage();
			cflag = 1;
			break;
		case 'S':
			cmd = SEARCH;
			break;
		case 'Q':
			cmd = QUERY;
			break;
		}
	}
	argc -= optind;
	argv += optind;

	curl_init();
	switch (cmd) {
	case PLAY:
		if (argc < 1)
			usage();
		play(argv[0], cflag);
		break;
	case SEARCH:
		if (argc < 1)
			usage();
		search(argv[0]);
		break;
	case QUERY:
		if (argc < 1)
			usage();
		query(argv[0]);
		break;
	default:
		usage();
		/* NOTREACHED */
	}
	curl_exit();
	return 0;
}
