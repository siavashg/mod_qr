#include "httpd.h"
#include "http_config.h"
#include "qr_generator.h"

static int mod_qr_method_handler (request_rec *r)
{
	char *referer;
	char fallback_url = "http://lilqr.com/";
	struct mem_encode *png;
	int i;
	uint32_t h; /* Used for URL hashing */
	char *qr_hash = {0}; /* URL hash */
	apr_time_t now = apr_time_now();
  char expires[APR_RFC822_DATE_LEN] = {0};

	if (!r->handler || strcmp(r->handler, "qr"))
		return DECLINED;

	if (r->method_number != M_GET)
		return HTTP_METHOD_NOT_ALLOWED;

  referer = (char *)apr_table_get(r->headers_in, "Referer");

	if (referer) {
		qrPNG(referer, png);
	} else {
		qrPNG(fallback_url, png);
	}

	png = (struct mem_encode *) malloc(sizeof(struct mem_encode));
	
	/* Generate hash for URL */
	h = hashlittle(referer, strlen(referer), h);
	asprintf(&qr_hash, "%.8x", h);

	/* Set Etag to URL hash */
	if(qr_hash) {
		apr_table_set(r->headers_out, "ETag", qr_hash);
		free(qr_hash);
	}

	/* Avoid all forms of caching */
	apr_table_set(r->headers_out, "Cache-Control", "private, no-cache, no-store, must-revalidate, max-age=0");
	apr_table_set(r->headers_out, "Pragma", "no-cache");
	apr_table_set(r->headers_out, "Expires", "Thu, 01 Jan 1970 00:00:00 GMT");
	apr_table_set(r->headers_out, "Vary", "*");
	
	/* Set Content-Type */
	ap_set_content_length(r, png->size);
	ap_set_content_type(r, "image/png;");

	/* Flush */
	for(i = 0; i < png->size; i++) {
		ap_rputc(png->buffer[i], r);
	}

	if(png->buffer)
		free(png->buffer);

	return OK;
}

static void mod_qr_register_hooks (apr_pool_t *p)
{
	ap_hook_handler(mod_qr_method_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA qr_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mod_qr_register_hooks,
};
