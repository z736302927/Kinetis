/**
 * @file http_server.c
 * @brief Lightweight HTTP File Server implementation
 *
 * Built on lwIP socket API (wiced_tcpip.h), processes one
 * connection per poll cycle.  Supports:
 *   - GET /           → HTML directory listing
 *   - GET /file.ext   → download with MIME type
 *   - POST /upload    → file upload (multipart/form-data or raw body)
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <kinetis/fatfs-intf.h>
#include "../fs/fatfs/ff.h"

#include "ap6181_lwip_port.h"
#include "wiced_tcpip.h"
#include "http_server.h"

/* ── Constants ────────────────────────────────────────── */

#define HTTP_MAX_URI        256
#define HTTP_MAX_HEADER     1024
#define HTTP_SEND_CHUNK     1024    /* Bytes per FATFS read chunk */
#define HTTP_MAX_CLIENTS    4       /* Max concurrent connections */

/* ── Structures ───────────────────────────────────────── */

struct http_client {
	int    fd;
	char   uri[HTTP_MAX_URI];
	char   method[8];       /* "GET" or "POST" */
	u32    content_length;
	u8     header_parsed;
	u8     sending;
	/* File transfer state */
	FIL    file;
	u8     file_open;
	u32    bytes_sent;
	u32    bytes_total;
	/* Upload state */
	u32    bytes_received;
	char   upload_name[64];
};

struct http_server {
	u16    port;
	int    listen_fd;
	char   root_path[64];
	struct http_client clients[HTTP_MAX_CLIENTS];
	u8     running;
};

/* ── MIME type guessing ──────────────────────────────── */

static const char *_mime_type(const char *path)
{
	const char *ext = strrchr(path, '.');
	if (!ext) return "application/octet-stream";

	if (!strcmp(ext, ".html") || !strcmp(ext, ".htm"))
		return "text/html";
	if (!strcmp(ext, ".css"))  return "text/css";
	if (!strcmp(ext, ".js"))   return "text/javascript";
	if (!strcmp(ext, ".txt") || !strcmp(ext, ".log"))
		return "text/plain";
	if (!strcmp(ext, ".json")) return "application/json";
	if (!strcmp(ext, ".xml"))  return "application/xml";
	if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg"))
		return "image/jpeg";
	if (!strcmp(ext, ".png"))  return "image/png";
	if (!strcmp(ext, ".gif"))  return "image/gif";
	if (!strcmp(ext, ".ico"))  return "image/x-icon";
	if (!strcmp(ext, ".svg"))  return "image/svg+xml";
	if (!strcmp(ext, ".pov"))  return "application/octet-stream";
	if (!strcmp(ext, ".bin"))  return "application/octet-stream";
	if (!strcmp(ext, ".hex"))  return "text/plain";
	return "application/octet-stream";
}

/* ── HTTP response helpers ───────────────────────────── */

static int _send_response(int fd, int code, const char *status,
	const char *mime, u32 content_len)
{
	char hdr[256];
	snprintf(hdr, sizeof(hdr),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %u\r\n"
		"Connection: close\r\n"
		"Server: Kinetis-POV/1.0\r\n"
		"\r\n",
		code, status,
		mime ? mime : "text/html",
		content_len);
	return wiced_tcp_send(fd, hdr, strlen(hdr));
}

static int _send_body(int fd, const char *data, u32 len)
{
	if (len == 0) return 0;
	return wiced_tcp_send(fd, data, len);
}

/* ── Directory listing ───────────────────────────────── */

static int _serve_directory(struct http_server *srv,
	struct http_client *cli)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	char html[2048];
	char line[128];
	int total = 0;
	int ret;

	res = f_opendir(&dir, srv->root_path);
	if (res != FR_OK) {
		_send_response(cli->fd, 500, "Internal Server Error",
			NULL, 0);
		return -EIO;
	}

	/* Build HTML header */
	total += snprintf(html + total, sizeof(html) - total,
		"<!DOCTYPE html><html><head>"
		"<meta charset=\"utf-8\">"
		"<title>Kinetis POV — Files</title>"
		"<style>body{font-family:monospace;"
		"max-width:800px;margin:20px auto;"
		"padding:0 10px;}"
		"table{width:100%%;border-collapse:collapse;}"
		"td,th{padding:8px;text-align:left;"
		"border-bottom:1px solid #ddd;}"
		"a{color:#2196F3;text-decoration:none;}"
		".upload{margin:20px 0;}"
		"</style></head><body>"
		"<h1>Kinetis POV — Files</h1>"
		"<p>Path: %s</p>"
		"<table><tr><th>Name</th><th>Size</th></tr>",
		srv->root_path);

	while (1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0)
			break;
		if (fno.fattrib & AM_SYS)
			continue;

		total += snprintf(line, sizeof(line),
			"<tr><td><a href=\"/%s\">%s%s</a></td>"
			"<td>%lu</td></tr>",
			fno.fname, fno.fname,
			(fno.fattrib & AM_DIR) ? "/" : "",
			(unsigned long)fno.fsize);

		if (total + (int)strlen(line) >= (int)sizeof(html))
			break;
		memcpy(html + total, line, strlen(line));
		total += strlen(line);
	}

	f_closedir(&dir);

	/* Upload form */
	total += snprintf(html + total, sizeof(html) - total,
		"</table>"
		"<div class=\"upload\">"
		"<h2>Upload File</h2>"
		"<form action=\"/upload\" method=\"post\""
		" enctype=\"multipart/form-data\">"
		"<input type=\"file\" name=\"file\">"
		"<input type=\"submit\" value=\"Upload\">"
		"</form></div>"
		"</body></html>");

	_send_response(cli->fd, 200, "OK", "text/html", total);
	ret = _send_body(cli->fd, html, total);

	return ret;
}

/* ── File download ───────────────────────────────────── */

static int _serve_file(struct http_server *srv,
	struct http_client *cli, const char *requested)
{
	char path[128];
	FRESULT res;

	/* Sanitize path — prevent directory traversal */
	{
		const char *p = requested;
		while (*p == '/') p++;
		if (strstr(p, "..") || strchr(p, '\\'))
			return -EINVAL;
		snprintf(path, sizeof(path), "%s/%s",
			srv->root_path, p);
	}

	res = f_open(&cli->file, path, FA_READ);
	if (res != FR_OK) {
		char body[128];
		int len = snprintf(body, sizeof(body),
			"<h1>404 Not Found</h1><p>File not found: %s</p>",
			p);
		_send_response(cli->fd, 404, "Not Found",
			"text/html", len);
		_send_body(cli->fd, body, len);
		return -ENOENT;
	}

	cli->file_open    = 1;
	cli->bytes_total  = f_size(&cli->file);
	cli->bytes_sent   = 0;
	cli->sending      = 1;

	_send_response(cli->fd, 200, "OK",
		_mime_type(path), cli->bytes_total);

	pr_info("http: serving '%s' (%u bytes)\n",
		p, cli->bytes_total);
	return 0;
}

/* ── File upload ─────────────────────────────────────── */

static int _handle_upload(struct http_server *srv,
	struct http_client *cli)
{
	char path[128];
	FRESULT res;
	UINT bw;

	if (!cli->upload_name[0]) {
		snprintf(cli->upload_name,
			sizeof(cli->upload_name), "upload.bin");
	}

	snprintf(path, sizeof(path), "%s/%s",
		srv->root_path, cli->upload_name);

	res = f_open(&cli->file, path,
		FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		char body[64];
		int len = snprintf(body, sizeof(body),
			"Upload failed: cannot create file");
		_send_response(cli->fd, 500,
			"Internal Server Error", "text/html", len);
		_send_body(cli->fd, body, len);
		return -EIO;
	}

	cli->file_open    = 1;
	cli->bytes_received = 0;

	pr_info("http: receiving upload '%s'\n", cli->upload_name);

	/* Read body data and write to file */
	{
		u8 buf[HTTP_SEND_CHUNK];
		int n;

		while (cli->bytes_received < cli->content_length) {
			u32 remain = cli->content_length -
				cli->bytes_received;
			u32 to_read = (remain > sizeof(buf)) ?
				sizeof(buf) : remain;

			n = wiced_tcp_receive(cli->fd, buf, to_read);
			if (n <= 0)
				break;

			res = f_write(&cli->file, buf, n, &bw);
			if (res != FR_OK || bw != (UINT)n)
				break;

			cli->bytes_received += n;
		}
	}

	f_close(&cli->file);
	cli->file_open = 0;

	{
		char body[128];
		int len = snprintf(body, sizeof(body),
			"<h1>Upload OK</h1>"
			"<p>File saved: %s (%u bytes)</p>"
			"<a href=\"/\">Back</a>",
			cli->upload_name, cli->bytes_received);
		_send_response(cli->fd, 200, "OK",
			"text/html", len);
		_send_body(cli->fd, body, len);
	}

	pr_info("http: upload complete, %u bytes\n",
		cli->bytes_received);
	return 0;
}

/* ── Parse HTTP header ───────────────────────────────── */

static int _parse_header(struct http_client *cli, char *buf, int len)
{
	char *save;
	char *line;
	char *uri;

	buf[len] = '\0';

	/* First line: "GET /path HTTP/1.1" */
	line = strtok_r(buf, "\r\n", &save);
	if (!line) return -EINVAL;

	/* Method */
	{
		char *sp = strchr(line, ' ');
		if (!sp) return -EINVAL;
		*sp = '\0';
		strncpy(cli->method, line, sizeof(cli->method) - 1);
		uri = sp + 1;
	}

	/* URI */
	{
		char *sp = strchr(uri, ' ');
		if (sp) *sp = '\0';
		strncpy(cli->uri, uri, sizeof(cli->uri) - 1);
	}

	/* Parse remaining headers for Content-Length */
	{
		char *hdr;
		while ((hdr = strtok_r(NULL, "\r\n", &save)) != NULL) {
			if (strncasecmp(hdr, "Content-Length:",
				15) == 0) {
				const char *val = hdr + 15;
				while (*val == ' ') val++;
				cli->content_length = simple_strtoul(val,
					NULL, 10);
			}
		}
	}

	cli->header_parsed = 1;
	return 0;
}

/* ── Init / Deinit ───────────────────────────────────── */

struct http_server *http_server_init(u16 port, const char *root_path)
{
	struct http_server *srv;

	srv = kzalloc(sizeof(*srv), GFP_KERNEL);
	if (!srv)
		return NULL;

	srv->port = port;
	strncpy(srv->root_path, root_path, sizeof(srv->root_path) - 1);

	srv->listen_fd = wiced_tcp_create_socket();
	if (srv->listen_fd < 0) {
		pr_err("http: socket create failed\n");
		kfree(srv);
		return NULL;
	}

	{
		wiced_ip_address_t any_addr;
		SET_IPV4_ADDRESS(any_addr, 0); /* INADDR_ANY */
		if (wiced_tcp_bind(srv->listen_fd, &any_addr,
			port) != WICED_SUCCESS) {
			pr_err("http: bind port %u failed\n", port);
			wiced_tcp_close(srv->listen_fd);
			kfree(srv);
			return NULL;
		}
	}

	if (wiced_tcp_listen(srv->listen_fd, HTTP_MAX_CLIENTS)
		!= WICED_SUCCESS) {
		pr_err("http: listen failed\n");
		wiced_tcp_close(srv->listen_fd);
		kfree(srv);
		return NULL;
	}

	srv->running = 1;
	pr_info("http: server started on port %u, root='%s'\n",
		port, root_path);
	return srv;
}

void http_server_deinit(struct http_server *srv)
{
	int i;
	if (!srv) return;

	srv->running = 0;

	for (i = 0; i < HTTP_MAX_CLIENTS; i++) {
		if (srv->clients[i].fd >= 0) {
			if (srv->clients[i].file_open)
				f_close(&srv->clients[i].file);
			wiced_tcp_close(srv->clients[i].fd);
		}
	}

	if (srv->listen_fd >= 0)
		wiced_tcp_close(srv->listen_fd);

	pr_info("http: server stopped\n");
	kfree(srv);
}

/* ── Poll ────────────────────────────────────────────── */

void http_server_poll(struct http_server *srv)
{
	int i, fd, ret;
	char rx_buf[HTTP_MAX_HEADER];

	if (!srv || !srv->running)
		return;

	/* ── Accept new connections ── */
	fd = wiced_tcp_accept(srv->listen_fd);
	if (fd >= 0) {
		/* Find free slot */
		for (i = 0; i < HTTP_MAX_CLIENTS; i++) {
			if (srv->clients[i].fd < 0) {
				srv->clients[i].fd = fd;
				srv->clients[i].header_parsed = 0;
				srv->clients[i].sending = 0;
				srv->clients[i].file_open = 0;
				srv->clients[i].bytes_sent = 0;
				memset(srv->clients[i].upload_name,
					0, sizeof(
					srv->clients[i].upload_name));
				break;
			}
		}
		if (i == HTTP_MAX_CLIENTS) {
			/* No slots — reject */
			const char *busy =
				"HTTP/1.1 503 Busy\r\n\r\n";
			wiced_tcp_send(fd, busy, strlen(busy));
			wiced_tcp_close(fd);
		}
	}

	/* ── Process connected clients ── */
	for (i = 0; i < HTTP_MAX_CLIENTS; i++) {
		struct http_client *cli = &srv->clients[i];
		if (cli->fd < 0)
			continue;

		/* Parse header */
		if (!cli->header_parsed) {
			ret = wiced_tcp_receive(cli->fd,
				(u8 *)rx_buf, sizeof(rx_buf) - 1);
			if (ret <= 0) {
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
				continue;
			}

			if (_parse_header(cli, rx_buf, ret) < 0) {
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
				continue;
			}

			/* Dispatch */
			if (strcmp(cli->method, "GET") == 0) {
				if (strcmp(cli->uri, "/") == 0) {
					_serve_directory(srv, cli);
				} else if (cli->uri[0] == '/') {
					_serve_file(srv, cli,
						cli->uri + 1);
				}
			} else if (strcmp(cli->method, "POST") == 0) {
				_handle_upload(srv, cli);
			} else {
				_send_response(cli->fd, 405,
					"Method Not Allowed", NULL, 0);
			}
		}

		/* Continue sending file chunks */
		if (cli->sending && cli->file_open) {
			u8 chunk[HTTP_SEND_CHUNK];
			UINT br;
			FRESULT res;

			u32 remain = cli->bytes_total -
				cli->bytes_sent;
			if (remain == 0) {
				f_close(&cli->file);
				cli->file_open = 0;
				cli->sending   = 0;
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
				continue;
			}

			u32 to_read = (remain > sizeof(chunk)) ?
				sizeof(chunk) : remain;

			res = f_read(&cli->file, chunk,
				to_read, &br);
			if (res != FR_OK || br == 0) {
				f_close(&cli->file);
				cli->file_open = 0;
				cli->sending   = 0;
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
				continue;
			}

			ret = wiced_tcp_send(cli->fd,
				chunk, br);
			if (ret <= 0) {
				f_close(&cli->file);
				cli->file_open = 0;
				cli->sending   = 0;
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
				continue;
			}

			cli->bytes_sent += br;

			/* If done, close */
			if (cli->bytes_sent >= cli->bytes_total) {
				f_close(&cli->file);
				cli->file_open = 0;
				cli->sending   = 0;
				wiced_tcp_close(cli->fd);
				cli->fd = -1;
			}
		}

		/* Close completed non-file responses */
		if (cli->header_parsed && !cli->sending &&
			!cli->file_open) {
			wiced_tcp_close(cli->fd);
			cli->fd = -1;
		}
	}
}
