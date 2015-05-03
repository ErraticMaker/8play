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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <json.h>

#include "8tracks.h"
#include "curl.h"

#define SERVERNAME	"https://8tracks.com/"

static size_t	intlen(int);
static struct	mix *mix_init(json_object *);
static int	statusok(json_object *);
static struct	track *track_get(const char *);
static struct	track *track_init(json_object *);
static void	*xmalloc(size_t);

char *
getplaytoken(void)
{
	struct json_object *root, *pt;
	char *js, *playtoken = NULL, url[] = SERVERNAME "sets/new";
	size_t len;

	js = curl_fetch(url, NULL);
	root = json_tokener_parse(js);
	if (root == NULL)
		goto end;
	if (!json_object_object_get_ex(root, "play_token", &pt))
		goto end;

	len = json_object_get_string_len(pt) + 1;
	playtoken = xmalloc(len);
	memcpy(playtoken, json_object_get_string(pt), len);
end:
	if (root)
		json_object_put(root);
	free(js);
	return playtoken;
}

/*
 * Returns the string size of an integer
 */
static size_t
intlen(int i)
{
	size_t len = 1;

	if (i < 0)
		i *= -1;
	while (i > 9) {
		len++;
		i /= 10;
	}
	return len;
}

void
mix_free(struct mix *mix)
{
	if (mix == NULL)
		return;
	free(mix->url);
	free(mix->name);
	free(mix->user);
	free(mix->description);
	free(mix->tags);
	free(mix->certification);
	free(mix);
}

struct mix *
mix_getbysimilar(int mixid, const char *playtoken)
{
	struct mix *m;
	struct json_object *mix, *root;
	char *js, *url;
	size_t len;
	int nr;

	len = strlen(SERVERNAME) + strlen("sets/") + strlen(playtoken) +
	    strlen("/next_mix?mix_id=") + intlen(mixid) +
	    strlen("&include=user") + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%ssets/%s/next_mix?mix_id=%d&include=user",
	    SERVERNAME, playtoken, mixid);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "next mix URL too long");

	js = curl_fetch(url, NULL);
	free(url);
	root = json_tokener_parse(js);
	if (root == NULL)
		goto error;
	if (!statusok(root))
		goto error;
	if (!json_object_object_get_ex(root, "next_mix", &mix))
		goto error;
	m = mix_init(mix);
	json_object_put(root);
	free(js);
	return m;
error:
	if (root)
		json_object_put(root);
	free(js);
	return NULL;
}

struct mix *
mix_getbyurl(const char *url)
{
	struct mix *m;
	struct json_object *mix, *root;
	char *js, *path;
	const char *p;
	size_t len;
	int nr;

	/*
	 * Check if the full URL is given or just the extension.
	 * Let p point to the extension.
	 */
	p = strstr(url, "http://8tracks.com/");
	if (p == &url[0])
		p += strlen("http://8tracks.com/");
	else if (p == NULL) {
		/* if http fails, check against https */
		p = strstr(url, "https://8tracks.com/");
		if (p == &url[0])
			p += strlen("https://8tracks.com/");
		else
			p = url;
	}

	len = strlen(SERVERNAME) + strlen(p) + 1;
	path = xmalloc(len);
	nr = snprintf(path, len, "%s%s", SERVERNAME, p);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "mix URL too long");

	js = curl_fetch(path, NULL);
	free(path);
	root = json_tokener_parse(js);
	if (root == NULL)
		goto error;
	if (!statusok(root))
		goto error;
	if (!json_object_object_get_ex(root, "mix", &mix))
		goto error;
	m = mix_init(mix);
	json_object_put(root);
	free(js);
	return m;
error:
	if (root)
		json_object_put(root);
	free(js);
	return NULL;
}

static struct mix *
mix_init(json_object *mix)
{
	json_object *certification, *description, *duration, *id,
	    *likescount, *name, *playscount, *tags, *trackscount, *url,
	    *user, *userid, *username;
	struct mix *m;
	size_t len;

	m = xmalloc(sizeof(struct mix));
	if (!json_object_object_get_ex(mix, "id", &id) ||
	    !json_object_object_get_ex(mix, "web_path", &url) ||
	    !json_object_object_get_ex(mix, "name", &name) ||
	    !json_object_object_get_ex(mix, "description", &description) ||
	    !json_object_object_get_ex(mix, "tag_list_cache", &tags) ||
	    !json_object_object_get_ex(mix, "certification", &certification) ||
	    !json_object_object_get_ex(mix, "likes_count", &likescount) ||
	    !json_object_object_get_ex(mix, "plays_count", &playscount) ||
	    !json_object_object_get_ex(mix, "tracks_count", &trackscount) ||
	    !json_object_object_get_ex(mix, "duration", &duration) ||
	    !json_object_object_get_ex(mix, "user", &user) ||
	    !json_object_object_get_ex(user, "id", &userid) ||
	    !json_object_object_get_ex(user, "login", &username))
		goto error;

	m->id = json_object_get_int(id);
	if (json_object_get_string(url) != NULL) {
		len = json_object_get_string_len(url) + 1;
		m->url = xmalloc(len);
		memcpy(m->url, json_object_get_string(url), len);
	} else
		m->url = NULL;
	if (json_object_get_string(name) != NULL) {
		len = json_object_get_string_len(name) + 1;
		m->name = xmalloc(len);
		memcpy(m->name, json_object_get_string(name), len);
	} else
		m->name = NULL;
	m->userid = json_object_get_int(userid);
	if (json_object_get_string(username) != NULL) {
		len = json_object_get_string_len(username) + 1;
		m->user = xmalloc(len);
		memcpy(m->user, json_object_get_string(username), len);
	} else
		m->user = NULL;
	if (json_object_get_string(description) != NULL) {
		len = json_object_get_string_len(description) + 1;
		m->description = xmalloc(len);
		memcpy(m->description, json_object_get_string(description),
		    len);
	} else
		m->description = NULL;
	if (json_object_get_string(tags) != NULL) {
		len = json_object_get_string_len(tags) + 1;
		m->tags = xmalloc(len);
		memcpy(m->tags, json_object_get_string(tags), len);
	} else
		m->tags = NULL;
	if (json_object_get_string(certification) != NULL) {
		len = json_object_get_string_len(certification) + 1;
		m->certification = xmalloc(len);
		memcpy(m->certification, json_object_get_string(certification),
		    len);
	} else
		m->certification = NULL;
	m->likescount = json_object_get_int(likescount);
	m->playscount = json_object_get_int(playscount);
	m->trackscount = json_object_get_int(trackscount);
	m->duration = json_object_get_int(duration);

	return m;
error:
	free(m);
	return NULL;
}

void
mixset_free(struct mix ***mix, size_t size)
{
	size_t i;

	for (i = 0; i < size; ++i)
		mix_free((*mix)[i]);
	free(*mix);
}

struct mix **
mixset_searchbysmartid(const char *smartid, size_t *size)
{
	struct mix **m;
	json_object *root = NULL, *mixset, *mixes;
	char *js, *url;
	size_t len;
	int i, nr;

	len = strlen(SERVERNAME) + strlen("mix_sets/") + strlen(smartid) +
	    strlen("?include=mixes[user]") + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%smix_sets/%s?include=mixes[user]", SERVERNAME,
	    smartid);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "search by smartid url too long");

	js = curl_fetch(url, NULL);
	free(url);
	if (js == NULL)
		goto error;
	root = json_tokener_parse(js);
	if (root == NULL)
		goto error;
	if (!statusok(root))
		goto error;
	if (!json_object_object_get_ex(root, "mix_set", &mixset) ||
	    !json_object_object_get_ex(mixset, "mixes", &mixes))
		goto error;
	*size = json_object_array_length(mixes);
	m = xmalloc(*size * sizeof(struct mix *));
	for (i = 0; (size_t)i < *size; ++i)
		m[i] = mix_init(json_object_array_get_idx(mixes, i));
	json_object_put(root);
	free(js);
	return m;
error:
	if (root)
		json_object_put(root);
	free(js);
	return NULL;
}

void
report(int trackid, int mixid, const char *playtoken)
{
	char *js, *url;
	size_t len;
	int nr;

	len = strlen(SERVERNAME) + strlen("sets/") + strlen(playtoken) +
	    strlen("/report?track_id=") + intlen(trackid) +
	    strlen("&mix_id=") + intlen(mixid) + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%ssets/%s/report?track_id=%d&mix_id=%d",
	    SERVERNAME, playtoken, trackid, mixid);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "report url too long");

	js = curl_fetch(url, NULL);

	free(js);
	free(url);
}

static int
statusok(json_object *root)
{
	json_object *status;
	int ret;

	if (json_object_object_get_ex(root, "status", &status) &&
	    strcmp("200 OK", json_object_get_string(status)) == 0)
		ret = 1;
	else
		ret = 0;
	return ret;
}

void
track_free(struct track *t)
{
	if (t == NULL)
		return;
	free(t->name);
	free(t->performer);
	free(t->url);
	free(t);
}

static struct track *
track_get(const char *url)
{
	struct track *t;
	json_object *root;
	char *js;

	js = curl_fetch(url, NULL);
	root = json_tokener_parse(js);
	if (root == NULL)
		goto error;
	if (!statusok(root))
		goto error;
	t = track_init(root);
	json_object_put(root);
	free(js);
	return t;
error:
	if (root)
		json_object_put(root);
	free(js);
	return NULL;
}

struct track *
track_getfirst(int mixid, const char *playtoken)
{
	struct track *track;
	char *url;
	size_t len;
	int nr;

	/* URL: 8tracks.com/sets/[playtoken]/play?mix_id=[mixid] */
	len = strlen(SERVERNAME) + strlen("sets/") + strlen(playtoken) +
	    strlen("/play?mix_id=") + intlen(mixid) + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%ssets/%s/play?mix_id=%d",
	    SERVERNAME, playtoken, mixid);
	if (nr == -1 || nr >= (int)len)
		errx(1, "first track URL too long");

	track = track_get(url);

	free(url);
	return track;
}

struct track *
track_getnext(int mixid, const char *playtoken)
{
	struct track *track;
	char *url;
	size_t len;
	int nr;

	/* URL: 8tracks.com/sets/[playtoken]/next?mix_id=[mixid] */
	len = strlen(SERVERNAME) + strlen("sets/") + strlen(playtoken) +
	    strlen("/next?mix_id=") + intlen(mixid) + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%ssets/%s/next?mix_id=%d",
	    SERVERNAME, playtoken, mixid);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "next track URL too long");

	track = track_get(url);

	free(url);
	return track;
}

struct track *
track_getskip(int mixid, const char *playtoken)
{
	struct track *track;
	char *url;
	size_t len;
	int nr;

	/* URL: 8tracks.com/sets/[playtoken]/skip?mix_id=[mixid] */
	len = strlen(SERVERNAME) + strlen("sets/") + strlen(playtoken) +
	    strlen("/skip?mix_id=") + intlen(mixid) + 1;
	url = xmalloc(len);
	nr = snprintf(url, len, "%ssets/%s/skip?mix_id=%d",
	    SERVERNAME, playtoken, mixid);
	if (nr == -1 || (size_t)nr >= len)
		errx(1, "skip track URL too long");

	track = track_get(url);

	free(url);
	return track;
}

static struct track *
track_init(json_object *root)
{
	json_object *id, *last, *name, *performer, *set, *skip, *track, *url;
	struct track *t;
	size_t len;

	t = xmalloc(sizeof(struct track));
	if (!json_object_object_get_ex(root, "set", &set) ||
	    !json_object_object_get_ex(set, "track", &track) ||
	    !json_object_object_get_ex(track, "id", &id) ||
	    !json_object_object_get_ex(set, "at_last_track", &last) ||
	    !json_object_object_get_ex(set, "skip_allowed", &skip) ||
	    !json_object_object_get_ex(track, "name", &name) ||
	    !json_object_object_get_ex(track, "performer", &performer) ||
	    !json_object_object_get_ex(track, "track_file_stream_url", &url))
		goto error;

	t->id = json_object_get_int(id);
	if (json_object_get_string(name) != NULL) {
		len = json_object_get_string_len(name) + 1;
		t->name = xmalloc(len);
		memcpy(t->name, json_object_get_string(name), len);
	} else
		t->name = NULL;
	if (json_object_get_string(performer) != NULL) {
		len = json_object_get_string_len(performer) + 1;
		t->performer = xmalloc(len);
		memcpy(t->performer, json_object_get_string(performer), len);
	} else
		t->performer = NULL;
	if (json_object_get_string(url) != NULL) {
		len = json_object_get_string_len(url) + 1;
		t->url = xmalloc(len);
		memcpy(t->url, json_object_get_string(url), len);
	} else
		t->url = NULL;
	t->lastflag = (int)json_object_get_boolean(last);
	t->skipallowedflag = (int)json_object_get_boolean(skip);
	return t;
error:
	free(t);
	return NULL;
}

static void *
xmalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p == NULL)
		err(1, NULL);
	return p;
}

