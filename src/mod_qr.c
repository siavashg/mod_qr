#include "httpd.h"
#include "http_config.h"
#include "qr_generator.h"

static int mod_qr_method_handler (request_rec *r)
{
	char *referer;
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

	if (!referer) {
		asprintf(&referer, "http://dev.lilqr.com/");
	}

	png = (struct mem_encode *) malloc(sizeof(struct mem_encode));
	qrPNG(referer, png);
	
	/* Generate hash for URL */
	h = hashlittle(referer, strlen(referer), h);
	asprintf(&qr_hash, "%.8x", h);

	fprintf(stderr,"apache2_mod_qr: QRGen Hash: '%s' URL: '%s'\n", qr_hash, referer);
	fflush(stderr);

	/* Set Etag to URL hash */
	if(qr_hash) {
		//apr_table_set(r->headers_out, "ETag", qr_hash);
		apr_table_set(r->headers_out, "ETag", referer);
		free(qr_hash);
	}

	/* Avoid all forms of caching */
	apr_table_set(r->headers_out, "Cache-Control", "private, no-cache, no-store, must-revalidate, max-age=0");
	apr_table_set(r->headers_out, "Pragma", "no-cache");
	/* Set Expires header to now */	
  //apr_rfc822_date(expires, now);
	//apr_table_set(r->headers_out, "Expires", expires);
	apr_table_set(r->headers_out, "Expires", "Thu, 01 Jan 1970 00:00:00 GMT");
	apr_table_set(r->headers_out, "Vary", "*");
	
	/* Set Content-Type */
	ap_set_content_length(r, png->size);
	ap_set_content_type(r, "image/png;");

	for(i = 0; i < png->size; i++) {
		ap_rputc(png->buffer[i], r);
	}

	if(png->buffer)
		free(png->buffer);

	return OK;
}

static int mod_qr_log_handler (request_rec *r)
{
	// Send a message to stderr (apache redirects this to the error log)
	// fprintf(stderr,"apache2_mod_qr: A QR code was generated.\n");

	// We need to flush the stream so that the message appears right away.
	// Performing an fflush() in a production system is not good for
	// performance - don't do this for real.
	// fflush(stderr);

	// Return DECLINED so that the Apache core will keep looking for
	// other modules to handle this request.  This effectively makes
	// this module completely transparent.
	return DECLINED;
}

/*
 * This function is a callback and it declares what other functions
 * should be called for request processing and configuration requests.
 * This callback function declares the Handlers for other events.
 */
static void mod_qr_register_hooks (apr_pool_t *p)
{
	// I think this is the call to make to register a handler for method calls (GET PUT et. al.).
	// We will ask to be last so that the comment has a higher tendency to
	// go at the end.
	ap_hook_handler(mod_qr_method_handler, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(mod_qr_log_handler, NULL, NULL, APR_HOOK_LAST);
}

/*
 * Declare and populate the module's data structure.  The
 * name of this structure ('mod_qr_module') is important - it
 * must match the name of the module.  This structure is the
 * only "glue" between the httpd core and the module.
 */
module AP_MODULE_DECLARE_DATA qr_module =
{
	// Only one callback function is provided.  Real
	// modules will need to declare callback functions for
	// server/directory configuration, configuration merging
	// and other tasks.
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mod_qr_register_hooks,			/* callback for registering hooks */
};
