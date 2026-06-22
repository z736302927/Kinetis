
#include <linux/list.h>
#include <linux/slab.h>

#include "fatfs/ff.h"

#include "lwip/tcp.h"
#include "lwip/err.h"

#include "../config.h"

#include "http_server.h"

#define HTTP_TRANSMIT_SIZE 2048
#define HTTP_RECEIVE_SIZE 1024

struct http_connection {
	struct list_head list;
	struct tcp_pcb *pcb;
	char transmit_buffer[HTTP_TRANSMIT_SIZE];
	char receive_buffer[HTTP_RECEIVE_SIZE];
	u32 transmit_length;
	u32 receive_length;
	u8 responded;
	u8 temp_link;
};

static LIST_HEAD(http_connection_head);

static struct tcp_pcb *tcp_pcb_listen;

static err_t http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct http_connection *connection;

	if (err != ERR_OK)
		return err;

	connection = kzalloc(sizeof(struct http_connection));
	if (!connection) {
		tcp_close(pcb);
		return ERR_MEM;
	}

	INIT_LIST_HEAD(&connection->list);
	connection->pcb = pcb;
	connection->temp_link = 1;

	list_add_tail(&connection->list, &http_connection_head);

	tcp_arg(pcb, connection);
	tcp_recv(pcb, http_receive_cb);
	tcp_sent(pcb, http_transmit_cb);
	tcp_err(pcb, http_error_cb);

	return ERR_OK;
}

static err_t http_receive_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct http_connection *connection = (struct http_connection *)arg;
	u32 copy_len;

	if (!connection || !pcb)
		return ERR_ARG;

	if (p == NULL || err != ERR_OK) {
		list_del_init(&connection->list);
		connection->pcb = NULL;
		tcp_close(pcb);
		kfree(connection);
		return ERR_OK;
	}

	copy_len = p->tot_len;
	if (connection->receive_length + copy_len > HTTP_RECEIVE_SIZE - 1)
		copy_len = HTTP_RECEIVE_SIZE - 1 - connection->receive_length;

	pbuf_copy_partial(p, &connection->receive_buffer[connection->receive_length], copy_len, 0);
	connection->receive_length += copy_len;
	connection->receive_buffer[connection->receive_length] = '\0';

	pbuf_free(p);
	tcp_recved(pcb, copy_len);

	if (strstr(connection->receive_buffer, "\r\n\r\n") || connection->receive_length >= HTTP_RECEIVE_SIZE - 1) {
		http_process_request(connection);
	}

	return ERR_OK;
}

static err_t http_transmit_cb(void *arg, struct tcp_pcb *pcb, u16 length)
{
	struct http_connection *connection = (struct http_connection *)arg;

	if (connection && connection->temp_link && connection->responded) {
		list_del_init(&connection->list);
		connection->pcb = NULL;
		tcp_close(pcb);
		kfree(connection);
	}

	return ERR_OK;
}

static void http_error_cb(void *arg, err_t err)
{
	struct http_connection *connection = (struct http_connection *)arg;

	if (connection && connection->pcb) {
		list_del_init(&connection->list);
		kfree(connection);
	}
}

static void http_process_request(struct http_connection *connection)
{
	char method[8];
	char path[256];
	char *body_start;

	if (sscanf(connection->receive_buffer, "%7s %255s", method, path) < 2) {
		http_send_response(connection, "400 Bad Request", "text/plain", "Bad Request", 11);
		return;
	}

	body_start = strstr(connection->receive_buffer, "\r\n\r\n");
	if (body_start)
		body_start += 4;

	if (strcmp(method, "GET") == 0) {
		if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
			http_send_file_list(connection);
		} else {
			http_send_file(connection, path + 1); /* strip '/' */
		}
	} else if (strcmp(method, "POST") == 0 && strcmp(path, "/upload") == 0) {
		http_handle_upload(connection, body_start, connection->receive_length - (body_start - connection->receive_buffer));
	} else {
		http_send_response(connection, "405 Method Not Allowed", "text/plain", "Method Not Allowed", 18);
	}
}

static void http_send_file_list(struct http_connection *connection)
{
	DIR dir;
	FILINFO fno;
	FRESULT res;
	int length;

	res = f_opendir(&dir, "0:");
	if (res != FR_OK) {
		http_send_response(connection, "500 Internal Server Error", "text/plain", "Cannot open SD card", 18);
		return;
	}

	length = snprintf(connection->transmit_buffer, HTTP_TRANSMIT_SIZE,
			"<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
			"<title>POV WiFi File Server</title>"
			"<style>body{font-family:Arial;max-width:800px;margin:20px auto;}"
			"table{width:100%%;border-collapse:collapse;}"
			"th,td{padding:8px 12px;text-align:left;border-bottom:1px solid #ddd;}"
			"tr:hover{background:#f5f5f5;}"
			"a{color:#0066cc;text-decoration:none;}"
			".upload{margin:20px 0;padding:15px;background:#f9f9f9;border-radius:5px;}"
			"input[type=file]{margin:10px 0;}"
			"input[type=submit]{padding:8px 20px;background:#0066cc;color:white;"
			"border:none;border-radius:3px;cursor:pointer;}"
			"</style></head><body>"
			"<h1>&#x1F4C1; POV WiFi File Server</h1>"
			"<div class=\"upload\"><form action=\"/upload\" method=\"post\" "
			"enctype=\"multipart/form-data\">"
			"<input type=\"file\" name=\"file\"><br>"
			"<input type=\"submit\" value=\"Upload\">"
			"</form></div>"
			"<table><tr><th>Name</th><th>Size</th></tr>");

	if (length < 0 || length >= HTTP_TRANSMIT_SIZE) {
		f_closedir(&dir);
		http_send_response(connection, "500 Internal Server Error", "text/plain", "Buffer overflow", 14);
		return;
	}

	while (1) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		if (fno.fattrib & (AM_DIR | AM_SYS))
			continue;

		length = strlen(connection->transmit_buffer);
		length += snprintf(connection->transmit_buffer + length, HTTP_TRANSMIT_SIZE - length, "<tr><td><a href=\"/%s\">%s</a></td><td>%lu</td></tr>",
				fno.fname, fno.fname, fno.fsize);
		if (length < 0 || length >= HTTP_TRANSMIT_SIZE)
			break;
	}

	f_closedir(&dir);

	length = strlen(connection->transmit_buffer);
	snprintf(connection->transmit_buffer + length, HTTP_TRANSMIT_SIZE - length, "</table></body></html>");

	http_send_response(connection, "200 OK", "text/html; charset=UTF-8", connection->transmit_buffer, strlen(connection->transmit_buffer));
}

static void http_send_file(struct http_connection *connection, const char *path)
{
	FIL file;
	FRESULT res;
	FSIZE_t size;
	char full_path[256];
	char header[256];
	int length;
	u32 br;

	snprintf(full_path, sizeof(full_path), "0:%s", path);

	res = f_open(&file, full_path, FA_READ);
	if (res != FR_OK) {
		http_send_response(connection, "404 Not Found", "text/plain", "File not found", 14);
		return;
	}

	size = f_size(&file);

	length = snprintf(header, sizeof(header),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Disposition: attachment; filename=\"%s\"\r\n"
		"Content-Length: %lu\r\n"
		"Connection: close\r\n"
		"\r\n",
		path, size);

	tcp_write(connection->pcb, header, length, TCP_WRITE_FLAG_COPY);

	do {
		res = f_read(&file, connection->transmit_buffer, HTTP_TRANSMIT_SIZE, &br);
		if (res != FR_OK || br == 0)
			break;
		tcp_write(connection->pcb, connection->transmit_buffer, br, TCP_WRITE_FLAG_COPY);
	} while (br == HTTP_TRANSMIT_SIZE);

	f_close(&file);

	tcp_output(connection->pcb);
	connection->responded = 1;
}

static void http_handle_upload(struct http_connection *connection, const char *data, int length)
{
	FIL file;
	FRESULT res;
	char upload_name[64];
	char msg[128];
	static int upload_counter;
	u32 size;

	snprintf(upload_name, sizeof(upload_name), "0:upload_%03d.bin", upload_counter++);

	res = f_open(&file, upload_name, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK) {
		http_send_response(connection, "500 Internal Server Error", "text/plain", "Failed to create file", 20);
		return;
	}

	if (length > 0 && data) {
		f_write(&file, data, length, &size);
	}

	f_close(&file);

	size = snprintf(msg, sizeof(msg),
		"<!DOCTYPE html><html><body><h1>Upload OK</h1>"
		"<p>Saved as %s (%d bytes)</p>"
		"<a href=\"/\">Back</a></body></html>",
		upload_name, length);
	http_send_response(connection, "200 OK", "text/html; charset=UTF-8", msg, size);
}

static void http_send_response(struct http_connection *connection, const char *status,
	const char *content_type, const char *body, int body_length)
{
	char header[512];
	int length;

	length = snprintf(header, sizeof(header),
		"HTTP/1.1 %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n",
		status, content_type, body_length);

	tcp_write(connection->pcb, header, length, TCP_WRITE_FLAG_COPY);

	if (body && body_length > 0)
		tcp_write(connection->pcb, body, body_length, TCP_WRITE_FLAG_COPY);

	tcp_output(connection->pcb);
	connection->responded = 1;
}

int http_server_init(void)
{
	tcp_pcb_listen = tcp_new();
	if (!tcp_pcb_listen)
		return -ENOMEM;

	tcp_bind(tcp_pcb_listen, IP_ADDR_ANY, WIFI_HTTP_PORT);

	tcp_pcb_listen = tcp_listen(tcp_pcb_listen);
	if (!tcp_pcb_listen)
		return -ENOMEM;

	tcp_accept(tcp_pcb_listen, http_accept);

	return 0;
}

void http_server_exit(void)
{
	struct http_connection *connection, *tmp;

	if (tcp_pcb_listen) {
		tcp_close(tcp_pcb_listen);
		tcp_pcb_listen = NULL;
	}

	list_for_each_entry_safe(connection, tmp, &http_connection_head, list) {
		struct tcp_pcb *pcb = connection->pcb;

		list_del_init(&connection->list);
		connection->pcb = NULL;
		tcp_close(pcb);
		kfree(connection);
	}
}
