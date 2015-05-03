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
#ifndef EIGHTTRACKS_H
#define EIGHTTRACKS_H

struct mix {
	int	id;
	char	*url;
	char	*name;
	char	*user;
	int	userid;
	char	*description;
	char	*tags;
	char	*certification;
	int	likescount;
	int	playscount;
	int	trackscount;
	int	duration;
};

struct track {
	int	id;
	char	*name;
	char	*performer;
	char	*url;
	int	lastflag;
	int	skipallowedflag;
};

__BEGIN_DECLS

char	*getplaytoken(void);
void	mix_free(struct mix *mix);
struct	mix *mix_getbysimilar(int mixid, const char *playtoken);
struct	mix *mix_getbyurl(const char *url);
void	mixset_free(struct mix ***mix, size_t size);
struct	mix **mixset_searchbysmartid(const char *smartid, size_t *size);
void	report(int trackid, int mixid, const char *playtoken);
void	track_free(struct track *track);
struct	track *track_getfirst(int mixid, const char *playtoken);
struct	track *track_getnext(int mixid, const char *playtoken);
struct	track *track_getskip(int mixid, const char *playtoken);

__END_DECLS

#endif	/* EIGHTTRACKS_H */

