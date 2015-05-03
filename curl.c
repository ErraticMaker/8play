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
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define APIKEY		"e233c13d38d96e3a3a0474723f6b3fcd21904979"
#define APIVERSION	3
#define USERAGENT	"8play"

struct curlbuf {
	char	*data;
	size_t	pos;
};

static size_t	curlwrite(void *, size_t, size_t, void *);

void
curl_init(void)
{
	CURLcode n;

	n = curl_global_init(CURL_GLOBAL_ALL);
	if (n != CURLE_OK) {
		errx(1, "curl_global_init failed, error: %s",
		    curl_easy_strerror(n));
	}
}

void
curl_exit(void)
{
	curl_global_cleanup();
}

static size_t
curlwrite(void *contents, size_t size, size_t nmemb, void *stream)
{
	struct curlbuf *b;
	size_t total;

	total = size * nmemb;
	if (total == 0)
		return 0;

	b = (struct curlbuf *)stream;
	b->data = realloc(b->data, b->pos + total + 1);
	if (b->data == NULL)
		err(1, NULL);
	memcpy(&(b->data[b->pos]), contents, total);
	b->pos += total;
	b->data[b->pos] = '\0';
	return total;
}

char *
curl_fetch(const char *url, const char *post)
{
	CURL *curl;
	CURLcode n;
	struct curl_slist *header;
	struct curlbuf buf = {.data = NULL, .pos = 0 };

	curl = curl_easy_init();
	if (curl == NULL)
		errx(1, "curl_easy_init failed");

	header = curl_slist_append(NULL, "X-Api-Key: " APIKEY);
	header = curl_slist_append(header, "X-Api-Version: 3");
	header = curl_slist_append(header, "User-Agent: " USERAGENT);
	header = curl_slist_append(header, "Accept: application/json");
	if (header == NULL)
		errx(1, "curl: set header failed");

	if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header) != 0 ||
	    curl_easy_setopt(curl, CURLOPT_URL, url) != 0 ||
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlwrite) != 0 ||
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf) != 0)
		errx(1, "curl_easy_setopt failed");
	if (post != NULL &&
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post) != 0)
		errx(1, "curl_easy_setopt failed");

	n = curl_easy_perform(curl);
	if (n != CURLE_OK)
		errx(1, "curl_easy_perform, error: %s", curl_easy_strerror(n));

	curl_easy_cleanup(curl);
	curl_slist_free_all(header);
	return buf.data;
}

