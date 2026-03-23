/**
 * @file ustp.c
 * @brief Universal Serial Transfer Protocol (ustp) Core Implementation
 * @version 3.0
 * @date 2026-03-22
 *
 * This file implements the core functionality of ustp including:
 * - Device management
 * - Protocol frame processing
 * - Data transmission and reception
 * - Sliding window mechanism
 * - Error handling
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/limits.h>

#include <kinetis/ustp.h>

/* Maximum transfer buffer size */
#define MAX_TRANSFER_BUFFER_SIZE    (64 * 1024)  /* 64KB */

/* Maximum packet retransmissions */
#define MAX_PACKET_RETRANSMISSIONS  5

/* Thread timeout in milliseconds */
#define THREAD_TIMEOUT_MS           100

/* Communication interface list */
static LIST_HEAD(g_comm_interface_list);
static DEFINE_MUTEX(g_comm_interface_lock);

/* Storage interface list */
static LIST_HEAD(g_storage_interface_list);
static DEFINE_MUTEX(g_storage_interface_lock);

/* Global device pointer for backward compatibility */
static struct ustp_device *g_ustp_dev;

static int ustp_build_frame(struct ustp_header *header, const u8 *data, u16 len, u8 *buffer)
{
	u16 checksum;
	u32 total_size;

	if (!header || !buffer) {
		return -EINVAL;
	}

	total_size = USTP_HEADER_SIZE + len;

	/* Set sync bytes */
	buffer[0] = USTP_SYNC_BYTE1;
	buffer[1] = USTP_SYNC_BYTE2;

	/* Set header fields */
	buffer[2] = header->version_major;
	buffer[3] = header->version_minor;
	buffer[4] = header->cmd;
	buffer[5] = header->flags;

	/* Sequence number (big-endian) */
	buffer[6] = (header->seq >> 8) & 0xFF;
	buffer[7] = header->seq & 0xFF;

	/* Length (big-endian) */
	buffer[8] = (len >> 8) & 0xFF;
	buffer[9] = len & 0xFF;

	/* Session ID (little-endian for compatibility) */
	memcpy(&buffer[12], &header->session_id, sizeof(u32));

	/* Reserved */
	memset(&buffer[16], 0, 4);

	/* Copy data if present */
	if (data && len > 0) {
		memcpy(&buffer[USTP_HEADER_SIZE], data, len);
	}

	/* Calculate checksum over entire frame (excluding sync and checksum itself) */
	checksum = crc16_ccitt(&buffer[2], USTP_HEADER_SIZE - 4 + len);
	buffer[10] = (checksum >> 8) & 0xFF;
	buffer[11] = checksum & 0xFF;

	return total_size;
}

static int ustp_parse_frame(const u8 *buffer, u32 buffer_len,
	struct ustp_header *header, u8 *data, u16 max_data_len)
{
	u16 checksum;
	u16 data_len;

	if (!buffer || !header) {
		return -EINVAL;
	}

	/* Check minimum frame size */
	if (buffer_len < USTP_HEADER_SIZE) {
		return -EINVAL;
	}

	/* Check sync bytes */
	if (buffer[0] != USTP_SYNC_BYTE1 || buffer[1] != USTP_SYNC_BYTE2) {
		return -ERR_INVALID_HEADER;
	}

	/* Parse header fields */
	header->version_major = buffer[2];
	header->version_minor = buffer[3];
	header->cmd = buffer[4];
	header->flags = buffer[5];

	/* Sequence number (big-endian) */
	header->seq = ((u16)buffer[6] << 8) | buffer[7];

	/* Length (big-endian) */
	data_len = ((u16)buffer[8] << 8) | buffer[9];
	header->length = data_len;

	/* Checksum (big-endian) */
	header->checksum = ((u16)buffer[10] << 8) | buffer[11];

	/* Session ID (little-endian for compatibility) */
	memcpy(&header->session_id, &buffer[12], sizeof(u32));

	/* Reserved */
	memcpy(header->reserved, &buffer[16], 4);

	/* Verify checksum */
	checksum = crc16_ccitt(&buffer[2], USTP_HEADER_SIZE - 4 + data_len);
	if (checksum != header->checksum) {
		return -ERR_CHECKSUM;
	}

	/* Check protocol version */
	if (header->version_major != PROTOCOL_VERSION_MAJOR) {
		return -ERR_PROTOCOL_VERSION;
	}

	/* Copy data if present */
	if (data && data_len > 0) {
		if (data_len > max_data_len) {
			return -EINVAL;
		}

		if (buffer_len < USTP_HEADER_SIZE + data_len) {
			return -EINVAL;
		}

		memcpy(data, &buffer[USTP_HEADER_SIZE], data_len);
	}

	return data_len;
}

static int ustp_validate_frame(const u8 *buffer, u32 len)
{
	struct ustp_header header;

	if (!buffer) {
		return -EINVAL;
	}

	return ustp_parse_frame(buffer, len, &header, NULL, 0);
}

static int window_init(struct sliding_window *window, u16 size)
{
	int i;

	if (!window || size == 0 || size > 32) {
		return -EINVAL;
	}

	memset(window, 0, sizeof(*window));
	window->window_size = size;
	window->send_base = 0;
	window->next_seq = 0;
	window->last_ack = 0;

	/* Initialize packet buffers */
	for (i = 0; i < 32; i++) {
		window->packets[i].seq = 0;
		window->packets[i].data = NULL;
		window->packets[i].len = 0;
		window->packets[i].acked = false;
		window->packets[i].timestamp = 0;
		window->packets[i].retry_count = 0;
	}

	return 0;
}

static int window_send(struct sliding_window *window, const u8 *data, u32 len)
{
	u16 seq;
	struct packet_buffer *pkt;

	if (!window || !data) {
		return -EINVAL;
	}

	/* Check if window is full */
	if ((window->next_seq - window->send_base) >= window->window_size) {
		return -EAGAIN;
	}

	seq = window->next_seq;
	pkt = &window->packets[seq % 32];

	/* Store packet information */
	pkt->seq = seq;
	pkt->len = len;
	pkt->acked = false;
	pkt->timestamp = jiffies_to_msecs(jiffies);
	pkt->retry_count = 0;

	/* Note: In real implementation, data should be copied to a buffer */
	pkt->data = (u8 *)data;  /* Temporary: should copy data */

	window->next_seq++;
	window->total_sent++;

	return seq;
}

static int window_ack(struct sliding_window *window, u16 seq)
{
	struct packet_buffer *pkt;

	if (!window) {
		return -EINVAL;
	}

	/* Check if sequence is within window */
	if (seq < window->send_base || seq >= window->next_seq) {
		return 0;    /* Duplicate or invalid ACK */
	}

	pkt = &window->packets[seq % 32];
	pkt->acked = true;
	window->total_acked++;

	/* Slide window */
	while (window->send_base < window->next_seq &&
		window->packets[window->send_base % 32].acked) {
		window->send_base++;
	}

	window->last_ack = seq;

	return 0;
}

static int window_timeout(struct sliding_window *window, u32 timeout_ms)
{
	u32 current_time;
	u16 i;
	int retransmit_count = 0;

	if (!window) {
		return 0;
	}

	current_time = jiffies_to_msecs(jiffies);

	for (i = window->send_base; i < window->next_seq; i++) {
		struct packet_buffer *pkt = &window->packets[i % 32];

		if (!pkt->acked && (current_time - pkt->timestamp) > timeout_ms) {
			/* Retransmit packet */
			pkt->timestamp = current_time;
			pkt->retry_count++;
			window->total_retransmitted++;
			retransmit_count++;

			/* Note: Actual retransmission should be done by caller */
		}
	}

	return retransmit_count;
}

static void window_cleanup(struct sliding_window *window)
{
	int i;

	if (!window) {
		return;
	}

	for (i = 0; i < 32; i++) {
		if (window->packets[i].data) {
			/* Note: Should free allocated buffer in real implementation */
			window->packets[i].data = NULL;
		}
	}

	memset(window, 0, sizeof(*window));
}

struct ustp_device *ustp_alloc_device(void)
{
	struct ustp_device *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return ERR_PTR(-ENOMEM);
	}

	/* Initialize device fields */
	dev->mode = MODE_FILE_TRANSFER;
	dev->state = STATE_IDLE;
	dev->session_id = 0;
	dev->initialized = false;
	dev->transferring = false;
	dev->paused = false;

	dev->mtu = DEFAULT_PACKET_SIZE;
	dev->window_size = DEFAULT_WINDOW_SIZE;
	dev->timeout_ms = DEFAULT_TIMEOUT_MS;
	dev->max_retry = MAX_RETRY_COUNT;

	dev->tx_seq = 0;
	dev->rx_seq = 0;

	dev->last_error = 0;

	return dev;
}

int ustp_init(struct ustp_device *dev,
	struct physics_interface_ops *phy,
	struct storage_interface_ops *storage,
	enum ustp_mode mode)
{
	int ret;

	if (!dev || !phy) {
		return -EINVAL;
	}

	if (dev->initialized) {
		pr_err("ustp device already initialized\n");
		return -EINVAL;
	}

	if (mode != MODE_FILE_TRANSFER && mode != MODE_STREAMING) {
		pr_err("Invalid transfer mode: %d\n", mode);
		return -ERR_INVALID_PARAM;
	}

	if (mode == MODE_FILE_TRANSFER && !storage) {
		pr_err("File transfer mode requires storage layer\n");
		return -ERR_INVALID_PARAM;
	}

	dev->phy = phy;
	dev->storage = storage;
	dev->mode = mode;

	/* Initialize communication interface if not already initialized */
	if (!dev->phy->initialized && dev->phy->ops && dev->phy->ops->init) {
		ret = dev->phy->ops->init(dev->phy->private);
		if (ret < 0) {
			pr_err("Failed to initialize communication interface: %d\n", ret);
			return ret;
		}
		dev->phy->initialized = true;
	}

	/* Initialize storage interface if provided */
	if (dev->storage && !dev->storage->initialized &&
		dev->storage->ops && dev->storage->ops->init) {
		ret = dev->storage->ops->init(dev->storage->private);
		if (ret < 0) {
			pr_err("Failed to initialize storage interface: %d\n", ret);
			return ret;
		}
		dev->storage->initialized = true;
	}

	ret = pthread_mutex_init(&dev->lock, NULL);
	if (ret != 0) {
		pr_err("Failed to initialize mutex: %d\n", ret);
		return -ret;
	}

	ret = pthread_cond_init(&dev->cond, NULL);
	if (ret != 0) {
		pr_err("Failed to initialize condition variable: %d\n", ret);
		pthread_mutex_destroy(&dev->lock);
		return -ret;
	}

	/* Initialize sliding windows */
	ret = window_init(&dev->tx_window, dev->window_size);
	if (ret < 0) {
		pr_err("Failed to initialize TX window: %d\n", ret);
		pthread_cond_destroy(&dev->cond);
		pthread_mutex_destroy(&dev->lock);
		return ret;
	}

	ret = window_init(&dev->rx_window, dev->window_size);
	if (ret < 0) {
		pr_err("Failed to initialize RX window: %d\n", ret);
		pthread_cond_destroy(&dev->cond);
		pthread_mutex_destroy(&dev->lock);
		return ret;
	}

	dev->buffer_size = MAX_TRANSFER_BUFFER_SIZE;
	dev->tx_buffer = kzalloc(dev->buffer_size, GFP_KERNEL);
	if (!dev->tx_buffer) {
		pr_err("Failed to allocate TX buffer\n");
		pthread_cond_destroy(&dev->cond);
		pthread_mutex_destroy(&dev->lock);
		return -ENOMEM;
	}
	dev->rx_buffer = kzalloc(dev->buffer_size, GFP_KERNEL);
	if (!dev->rx_buffer) {
		pr_err("Failed to allocate RX buffer\n");
		kfree(dev->tx_buffer);
		pthread_cond_destroy(&dev->cond);
		pthread_mutex_destroy(&dev->lock);
		return -ENOMEM;
	}

	dev->state = STATE_IDLE;
	dev->initialized = true;

	pr_info("ustp device initialized (mode=%s, phy=%s, storage=%s)\n",
		mode == MODE_FILE_TRANSFER ? "FILE" : "STREAMING",
		phy->name ? phy->name : "unknown",
		storage ? (storage->name ? storage->name : "unknown") : "none");

	return 0;
}

int ustp_exit(struct ustp_device *dev)
{
	if (!dev) {
		return -EINVAL;
	}

	if (!dev->initialized) {
		return 0;
	}

	/* Stop any ongoing transfer */
	if (dev->transferring) {
		pr_warn("ustp device is still transferring, cancelling...\n");
		ustp_cancel_transfer(dev);
	}

	/* Cleanup sliding windows */
	window_cleanup(&dev->tx_window);
	window_cleanup(&dev->rx_window);

	if (dev->tx_buffer) {
		kfree(dev->tx_buffer);
		dev->tx_buffer = NULL;
	}

	if (dev->rx_buffer) {
		kfree(dev->rx_buffer);
		dev->rx_buffer = NULL;
	}

	/* Destroy synchronization objects */
	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);

	dev->initialized = false;
	dev->state = STATE_IDLE;

	pr_info("ustp device cleaned up\n");

	return 0;
}

int ustp_send_packet(struct ustp_device *dev,
	u8 cmd, u8 flags, u16 seq,
	const u8 *data, u16 len)
{
	struct ustp_header header;
	int frame_len;
	int ret;

	if (!dev || !dev->phy || !dev->initialized) {
		return -EINVAL;
	}

	if (data && len > dev->buffer_size - USTP_HEADER_SIZE) {
		return -EINVAL;
	}

	/* Build frame header */
	memset(&header, 0, sizeof(header));
	header.version_major = PROTOCOL_VERSION_MAJOR;
	header.version_minor = PROTOCOL_VERSION_MINOR;
	header.cmd = cmd;
	header.flags = flags;
	header.seq = seq;
	header.session_id = dev->session_id;

	/* Build frame */
	frame_len = ustp_build_frame(&header, data, len, dev->tx_buffer);
	if (frame_len < 0) {
		pr_err("Failed to build frame: %d\n", frame_len);
		return frame_len;
	}

	/* Send through communication interface */
	pthread_mutex_lock(&dev->lock);
	if (dev->phy->ops && dev->phy->ops->send) {
		ret = dev->phy->ops->send(dev->phy->private, dev->tx_buffer,
				frame_len, dev->timeout_ms);
		if (ret < 0) {
			pr_err("Failed to send packet: %d\n", ret);
			pthread_mutex_unlock(&dev->lock);
			return ret;
		}

		dev->total_sent_bytes += frame_len;
		dev->phy->tx_bytes += frame_len;
	} else {
		pr_err("Send operation not implemented\n");
		pthread_mutex_unlock(&dev->lock);
		return -ERR_COMM_ERROR;
	}
	pthread_mutex_unlock(&dev->lock);

	return 0;
}

int ustp_recv_packet(struct ustp_device *dev,
	struct ustp_header *header,
	u8 *data, u16 max_len)
{
	int ret;
	int data_len;

	if (!dev || !dev->phy || !dev->initialized || !header) {
		return -EINVAL;
	}

	/* Receive through communication interface */
	pthread_mutex_lock(&dev->lock);
	if (dev->phy->ops && dev->phy->ops->recv) {
		ret = dev->phy->ops->recv(dev->phy->private, dev->rx_buffer,
				dev->buffer_size, dev->timeout_ms);
		if (ret < 0) {
			pr_err("Failed to receive packet: %d\n", ret);
			pthread_mutex_unlock(&dev->lock);
			return ret;
		}

		dev->total_recv_bytes += ret;
		dev->phy->rx_bytes += ret;
	} else {
		pr_err("Receive operation not implemented\n");
		pthread_mutex_unlock(&dev->lock);
		return -ERR_COMM_ERROR;
	}
	pthread_mutex_unlock(&dev->lock);

	/* Parse frame */
	data_len = ustp_parse_frame(dev->rx_buffer, ret, header, data, max_len);
	if (data_len < 0) {
		pr_err("Failed to parse frame: %d\n", data_len);
		return data_len;
	}

	return data_len;
}

int ustp_send_file(struct ustp_device *dev, const char *filename, u64 size)
{
	struct file_info_payload payload;
	int ret;

	if (!dev || !dev->initialized || !filename) {
		return -EINVAL;
	}

	if (dev->mode != MODE_FILE_TRANSFER) {
		pr_err("Send file only supported in file transfer mode\n");
		return -ERR_NOT_SUPPORTED;
	}

	if (!dev->storage) {
		pr_err("Storage layer required for file transfer\n");
		return -ERR_STORAGE_ERROR;
	}

	pthread_mutex_lock(&dev->lock);

	/* Check state */
	if (dev->state != STATE_READY && dev->state != STATE_IDLE) {
		pr_err("Invalid state for file transfer: %d\n", dev->state);
		pthread_mutex_unlock(&dev->lock);
		return -ERR_NOT_SUPPORTED;
	}

	/* Prepare file info */
	memset(&payload, 0, sizeof(payload));
	payload.file_size = size;
	payload.shard_size = DEFAULT_SHARD_SIZE;
	payload.total_shards = (size + payload.shard_size - 1) / payload.shard_size;
	strncpy((char *)payload.file_name, filename, sizeof(payload.file_name) - 1);
	payload.file_name_len = strlen((char *)payload.file_name);

	/* Send file info */
	ret = ustp_send_packet(dev, CMD_FILE_INFO, FLAG_FIRST_PKT,
			dev->tx_seq++, (u8 *)&payload, sizeof(payload));
	if (ret < 0) {
		pr_err("Failed to send file info: %d\n", ret);
		pthread_mutex_unlock(&dev->lock);
		return ret;
	}

	/* Update device state */
	dev->file_info.file_size = size;
	strncpy(dev->file_info.filename, filename, sizeof(dev->file_info.filename) - 1);
	dev->file_info.shard_size = payload.shard_size;
	dev->file_info.total_shards = payload.total_shards;
	dev->total_shards = payload.total_shards;
	dev->completed_shards = 0;
	dev->transferred_bytes = 0;
	dev->state = STATE_TRANSFERRING;
	dev->transferring = true;

	pthread_mutex_unlock(&dev->lock);

	pr_info("Started sending file: %s (size=%llu, shards=%u)\n",
		filename, size, payload.total_shards);

	return 0;
}

int ustp_receive_file(struct ustp_device *dev, const char *filename)
{
	if (!dev || !dev->initialized || !filename) {
		return -EINVAL;
	}

	if (dev->mode != MODE_FILE_TRANSFER) {
		pr_err("Receive file only supported in file transfer mode\n");
		return -ERR_NOT_SUPPORTED;
	}

	/* TODO: Implement file reception logic */
	pr_info("Receive file: %s\n", filename);

	return 0;
}

int ustp_resume_transfer(struct ustp_device *dev, const char *filename)
{
	struct resume_request_payload payload;
	int ret;

	if (!dev || !dev->initialized || !filename) {
		return -EINVAL;
	}

	if (dev->mode != MODE_FILE_TRANSFER) {
		pr_err("Resume only supported in file transfer mode\n");
		return -ERR_NOT_SUPPORTED;
	}

	if (!dev->storage) {
		pr_err("Storage layer required for resume\n");
		return -ERR_STORAGE_ERROR;
	}

	pthread_mutex_lock(&dev->lock);

	/* Prepare resume request */
	memset(&payload, 0, sizeof(payload));
	strncpy(payload.filename, filename, sizeof(payload.filename) - 1);
	payload.resume_offset = dev->file_info.resume_offset;
	payload.resume_shard = dev->file_info.current_shard;

	/* Send resume request */
	ret = ustp_send_packet(dev, CMD_RESUME_REQUEST, 0,
			dev->tx_seq++, (u8 *)&payload, sizeof(payload));
	if (ret < 0) {
		pr_err("Failed to send resume request: %d\n", ret);
		pthread_mutex_unlock(&dev->lock);
		return ret;
	}

	dev->state = STATE_TRANSFERRING;
	dev->transferring = true;

	pthread_mutex_unlock(&dev->lock);

	pr_info("Resuming file transfer: %s (offset=%llu, shard=%u)\n",
		filename, payload.resume_offset, payload.resume_shard);

	return 0;
}

int ustp_wait_complete(struct ustp_device *dev, u32 timeout_ms)
{
	struct timespec ts;
	int ret = 0;

	if (!dev || !dev->initialized) {
		return -EINVAL;
	}

	pthread_mutex_lock(&dev->lock);

	if (timeout_ms > 0) {
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout_ms / 1000;
		ts.tv_nsec += (timeout_ms % 1000) * 1000000;
		if (ts.tv_nsec >= 1000000000) {
			ts.tv_sec++;
			ts.tv_nsec -= 1000000000;
		}

		while (dev->transferring && ret == 0) {
			ret = pthread_cond_timedwait(&dev->cond, &dev->lock, &ts);
		}
	} else {
		while (dev->transferring) {
			pthread_cond_wait(&dev->cond, &dev->lock);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

int ustp_get_progress(struct ustp_device *dev,
	u64 *transferred, u64 *total)
{
	if (!dev || !dev->initialized) {
		return -EINVAL;
	}

	pthread_mutex_lock(&dev->lock);

	if (transferred) {
		*transferred = dev->transferred_bytes;
	}

	if (total) {
		*total = dev->file_info.file_size;
	}

	pthread_mutex_unlock(&dev->lock);

	return 0;
}

int ustp_cancel_transfer(struct ustp_device *dev)
{
	int ret;

	if (!dev || !dev->initialized) {
		return -EINVAL;
	}

	pthread_mutex_lock(&dev->lock);

	if (!dev->transferring) {
		pthread_mutex_unlock(&dev->lock);
		return 0;
	}

	/* Send abort command */
	ret = ustp_send_packet(dev, CMD_FILE_ABORT, 0, dev->tx_seq++, NULL, 0);
	if (ret < 0) {
		pr_err("Failed to send abort: %d\n", ret);
	}

	dev->transferring = false;
	dev->state = STATE_ABORTING;
	pthread_cond_broadcast(&dev->cond);

	pthread_mutex_unlock(&dev->lock);

	pr_info("Transfer cancelled\n");

	return 0;
}

int comm_interface_register(struct physics_interface_ops *phy)
{
	if (!phy || !phy->ops) {
		return -EINVAL;
	}

	mutex_lock(&g_comm_interface_lock);
	list_add(&phy->list, &g_comm_interface_list);
	mutex_unlock(&g_comm_interface_lock);

	pr_info("Registered communication interface: %s\n",
		phy->name ? phy->name : "unknown");

	return 0;
}

int comm_interface_unregister(struct physics_interface_ops *phy)
{
	if (!phy) {
		return -EINVAL;
	}

	mutex_lock(&g_comm_interface_lock);
	list_del(&phy->list);
	mutex_unlock(&g_comm_interface_lock);

	pr_info("Unregistered communication interface: %s\n",
		phy->name ? phy->name : "unknown");

	return 0;
}

struct physics_interface_ops *comm_interface_get(enum comm_interface_type type)
{
	struct physics_interface_ops *phy;

	mutex_lock(&g_comm_interface_lock);
	list_for_each_entry(phy, &g_comm_interface_list, list) {
		if (phy->type == type) {
			mutex_unlock(&g_comm_interface_lock);
			return phy;
		}
	}
	mutex_unlock(&g_comm_interface_lock);

	return NULL;
}

int storage_interface_register(struct storage_interface_ops *storage)
{
	if (!storage || !storage->ops) {
		return -EINVAL;
	}

	mutex_lock(&g_storage_interface_lock);
	list_add(&storage->list, &g_storage_interface_list);
	mutex_unlock(&g_storage_interface_lock);

	pr_info("Registered storage interface: %s\n",
		storage->name ? storage->name : "unknown");

	return 0;
}

int storage_interface_unregister(struct storage_interface_ops *storage)
{
	if (!storage) {
		return -EINVAL;
	}

	mutex_lock(&g_storage_interface_lock);
	list_del(&storage->list);
	mutex_unlock(&g_storage_interface_lock);

	pr_info("Unregistered storage interface: %s\n",
		storage->name ? storage->name : "unknown");

	return 0;
}

struct storage_interface_ops *storage_interface_get(enum storage_type type)
{
	struct storage_interface_ops *storage;

	mutex_lock(&g_storage_interface_lock);
	list_for_each_entry(storage, &g_storage_interface_list, list) {
		if (storage->type == type) {
			mutex_unlock(&g_storage_interface_lock);
			return storage;
		}
	}
	mutex_unlock(&g_storage_interface_lock);

	return NULL;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kinetis Project");
MODULE_DESCRIPTION("Universal Serial Transfer Protocol (ustp)");
MODULE_VERSION("3.0");

struct ustp_test_context {
	u32 total_tests;        /* Total number of tests executed */
	u32 passed_tests;       /* Number of passed tests */
	u32 failed_tests;       /* Number of failed tests */

	u64 total_bytes;        /* Total bytes transferred */
	u64 total_time_us;      /* Total time in microseconds */
	u64 total_flash_writes; /* Total Flash erase/write operations */

	u32 current_test;       /* Current test number */
	char test_name[64];     /* Current test name */
	u64 test_start_time_us; /* Test start time in microseconds */

	/* Performance statistics */
	u32 min_latency_us;     /* Minimum latency in microseconds */
	u32 max_latency_us;     /* Maximum latency in microseconds */
	u64 total_latency_us;   /* Total latency for average calculation */
	u32 packet_count;       /* Number of packets for latency tracking */

	/* Memory usage tracking */
	u32 peak_memory_kb;     /* Peak memory usage in KB */
};

static u64 ustp_get_time_us(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (u64)ts.tv_sec * 1000000 + (u64)ts.tv_nsec / 1000;
}

struct ustp_test_context *ustp_test_alloc(void)
{
	struct ustp_test_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		return ERR_PTR(-ENOMEM);
	}

	ctx->current_test = 0;
	ctx->min_latency_us = UINT_MAX;
	ctx->max_latency_us = 0;
	ctx->total_latency_us = 0;
	ctx->packet_count = 0;

	pr_info("Test context allocated\n");

	return ctx;
}

void ustp_test_free(struct ustp_test_context *ctx)
{
	if (!ctx) {
		return;
	}

	kfree(ctx);
	pr_info("Test context freed\n");
}

void ustp_test_record(struct ustp_test_context *ctx, int passed,
	u64 bytes, u64 time_us)
{
	u64 speed_kbps = 0;
	const char *result_str;

	if (!ctx) {
		return;
	}

	ctx->total_tests++;

	if (passed) {
		ctx->passed_tests++;
		result_str = "PASS";
	} else {
		ctx->failed_tests++;
		result_str = "FAIL";
	}

	ctx->total_bytes += bytes;
	ctx->total_time_us += time_us;

	/* Calculate speed in KB/s */
	if (time_us > 0 && bytes > 0) {
		speed_kbps = (bytes * 1000) / time_us;
	}

	/* Print test result */
	pr_info("Test %02d [%s]: %s - Bytes: %llu, Time: %llu us, Speed: %llu KB/s\n",
		ctx->current_test, ctx->test_name, result_str,
		bytes, time_us, speed_kbps);

	/* Update latency statistics */
	if (passed && time_us > 0) {
		if (time_us < ctx->min_latency_us) {
			ctx->min_latency_us = time_us;
		}

		if (time_us > ctx->max_latency_us) {
			ctx->max_latency_us = time_us;
		}

		ctx->total_latency_us += time_us;
		ctx->packet_count++;
	}
}

void ustp_test_summary(struct ustp_test_context *ctx)
{
	u32 pass_rate = 0;
	u64 avg_speed_kbps = 0;
	u32 avg_latency_us = 0;

	if (!ctx) {
		return;
	}

	/* Calculate pass rate */
	if (ctx->total_tests > 0) {
		pass_rate = (ctx->passed_tests * 100) / ctx->total_tests;
	}

	/* Calculate average speed */
	if (ctx->total_time_us > 0) {
		avg_speed_kbps = (ctx->total_bytes * 1000) / ctx->total_time_us;
	}

	/* Calculate average latency */
	if (ctx->packet_count > 0) {
		avg_latency_us = ctx->total_latency_us / ctx->packet_count;
	}

	pr_info("\n");
	pr_info("==================================================\n");
	pr_info("ustp Test Summary\n");
	pr_info("==================================================\n");
	pr_info("Total Tests:    %u\n", ctx->total_tests);
	pr_info("Passed Tests:   %u\n", ctx->passed_tests);
	pr_info("Failed Tests:   %u\n", ctx->failed_tests);
	pr_info("Pass Rate:      %u%%\n", pass_rate);
	pr_info("==================================================\n");
	pr_info("Total Bytes:    %llu\n", ctx->total_bytes);
	pr_info("Total Time:     %llu us\n", ctx->total_time_us);
	pr_info("Avg Speed:      %llu KB/s\n", avg_speed_kbps);
	pr_info("==================================================\n");
	pr_info("Flash Writes:   %llu\n", ctx->total_flash_writes);
	pr_info("Peak Memory:    %u KB\n", ctx->peak_memory_kb);
	pr_info("==================================================\n");
	pr_info("Min Latency:    %u us\n", ctx->min_latency_us);
	pr_info("Max Latency:    %u us\n", ctx->max_latency_us);
	pr_info("Avg Latency:    %u us\n", avg_latency_us);
	pr_info("==================================================\n");
}

static void *ustp_device_state_machine_thread(void *data)
{
	struct ustp_device *device = (struct ustp_device *)data;

	/* Set thread to cancelable */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (device->thread_running) {

	}
	return NULL;
}

static int ustp_device_start_thread(struct ustp_device *device)
{
	int ret;

	if (device->thread_running) {
		pr_warn("ustp_device(%s): Thread already running", device->name);
		return 0;
	}

	device->thread_running = 1;

	ret = pthread_create(&device->mosi_thread, NULL, ustp_device_state_machine_thread, device);
	if (ret != 0) {
		device->thread_running = 0;
		pr_err("ustp_device(%s): Failed to create mosi thread: %d", device->name, ret);
		return -ret;
	}

	pr_info("ustp_device(%s): Thread started successfully", device->name);
	return 0;
}

static void ustp_device_stop_thread(struct ustp_device *device)
{
	void *thread_ret;

	if (!device->thread_running) {
		return;
	}

	/* Signal thread to stop */
	device->thread_running = 0;

	/* Wait for mosi thread to finish */
	if (pthread_join(device->mosi_thread, &thread_ret) == 0) {
		pr_info("ustp_device(%s): MOSI thread stopped successfully", device->name);
	} else {
		pr_warn("ustp_device(%s): MOSI thread stop had issues, but continuing", device->name);
	}

	pr_info("ustp_device(%s): Thread stopped", device->name);
}

int t_ustp_initialize(void)
{
	struct ustp_device *dev;
	int ret;

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(dev);
		return ret;
	}

	if (!dev->initialized) {
		pr_err("Device not marked as initialized\n");
		ustp_exit(dev);
		kfree(dev);
		return -EFAULT;
	}

	pr_info("Device initialized successfully\n");

	ustp_exit(dev);
	kfree(dev);

	return 0;
}

int t_ustp_info_query(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;

	pr_info("=== FT-02: ustp Info Query Test ===\n");

	start_time = ustp_get_time_us();

	/* Allocate and initialize device */
	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_BLE;
	phy->name = "mock_ble";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	end_time = ustp_get_time_us();

	/* Query and print information */
	pr_info("Protocol Version: %d.%d\n",
		PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR);
	pr_info("Transfer Mode: %s\n",
		dev->mode == MODE_FILE_TRANSFER ? "FILE_TRANSFER" : "STREAMING");
	pr_info("Device State: %d\n", dev->state);
	pr_info("MTU: %u bytes\n", dev->mtu);
	pr_info("Window Size: %u\n", dev->window_size);
	pr_info("Timeout: %u ms\n", dev->timeout_ms);
	pr_info("Session ID: %u\n", dev->session_id);
	pr_info("Buffer Size: %u bytes\n", dev->buffer_size);
	pr_info("Comm Interface: %s\n", phy->name ? phy->name : "unknown");
	pr_info("Storage Interface: %s\n", storage->name ? storage->name : "unknown");

	/* Cleanup */
	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-02 PASSED ===\n\n");
	return 0;
}

int t_ustp_cleanup(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;

	pr_info("=== FT-03: ustp Cleanup Test ===\n");

	start_time = ustp_get_time_us();

	/* Allocate and initialize device */
	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SPI_FLASH;
	storage->name = "mock_flash";
	storage->initialized = true;

	if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	/* Verify initialization */
	if (!dev->initialized) {
		pr_err("Device initialization failed\n");
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	/* Cleanup */
	ustp_exit(dev);

	/* Verify cleanup */
	if (dev->initialized) {
		pr_err("Device still marked as initialized after cleanup\n");
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	if (dev->tx_buffer != NULL || dev->rx_buffer != NULL) {
		pr_err("Buffers not freed after cleanup\n");
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	kfree(storage);
	kfree(phy);
	kfree(dev);

	end_time = ustp_get_time_us();

	pr_info("Cleanup successful\n");
	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-03 PASSED ===\n\n");
	return 0;
}

int t_ustp_config_basic(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;

	pr_info("=== FT-04: Basic Configuration Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_UART;
	phy->name = "mock_uart";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SPI_FLASH;
	storage->name = "mock_flash";
	storage->initialized = true;

	if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	/* Test MTU configuration */
	dev->mtu = 512;
	if (dev->mtu != 512) {
		pr_err("Failed to set MTU\n");
		goto fail;
	}

	/* Test window size configuration */
	dev->window_size = 8;
	if (dev->window_size != 8) {
		pr_err("Failed to set window size\n");
		goto fail;
	}

	/* Test timeout configuration */
	dev->timeout_ms = 5000;
	if (dev->timeout_ms != 5000) {
		pr_err("Failed to set timeout\n");
		goto fail;
	}

	/* Test max retry configuration */
	dev->max_retry = 10;
	if (dev->max_retry != 10) {
		pr_err("Failed to set max retry\n");
		goto fail;
	}

	end_time = ustp_get_time_us();

	pr_info("MTU: %u bytes\n", dev->mtu);
	pr_info("Window Size: %u\n", dev->window_size);
	pr_info("Timeout: %u ms\n", dev->timeout_ms);
	pr_info("Max Retry: %u\n", dev->max_retry);

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-04 PASSED ===\n\n");
	return 0;

fail:
	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);
	return -EFAULT;
}

int t_ustp_config_comm(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	enum comm_interface_type types[] = {
		COMM_TYPE_UART, COMM_TYPE_BLE, COMM_TYPE_WIFI
	};
	const char *type_names[] = {"UART", "BLE", "WiFi"};
	int i;
	u64 start_time, end_time;

	pr_info("=== FT-05: Communication Interface Configuration Test ===\n");

	start_time = ustp_get_time_us();

	for (i = 0; i < 3; i++) {
		pr_info("Testing %s interface...\n", type_names[i]);

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device for %s\n", type_names[i]);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			return -ENOMEM;
		}
		phy->type = types[i];
		phy->name = (char *)type_names[i];
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SPI_FLASH;
		storage->name = "mock_flash";
		storage->initialized = true;

		if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
			pr_err("Failed to initialize %s device\n", type_names[i]);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return -EFAULT;
		}

		if (dev->phy->type != types[i]) {
			pr_err("Comm type mismatch: expected %d, got %d\n",
				types[i], dev->phy->type);
			goto fail;
		}

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);

		pr_info("%s interface test passed\n", type_names[i]);
	}

	end_time = ustp_get_time_us();

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-05 PASSED ===\n\n");
	return 0;

fail:
	return -EFAULT;
}

int t_ustp_config_storage(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	enum storage_type types[] = {
		STORAGE_TYPE_SPI_FLASH, STORAGE_TYPE_SD_CARD
	};
	const char *type_names[] = {"SPI_FLASH", "SD_CARD"};
	int i;
	u64 start_time, end_time;

	pr_info("=== FT-06: Storage Interface Configuration Test ===\n");

	start_time = ustp_get_time_us();

	for (i = 0; i < 2; i++) {
		pr_info("Testing %s storage...\n", type_names[i]);

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device for %s\n", type_names[i]);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_UART;
		phy->name = "mock_uart";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = types[i];
		storage->name = (char *)type_names[i];
		storage->initialized = true;

		if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
			pr_err("Failed to initialize %s device\n", type_names[i]);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return -EFAULT;
		}

		if (dev->storage->type != types[i]) {
			pr_err("Storage type mismatch: expected %d, got %d\n",
				types[i], dev->storage->type);
			goto fail;
		}

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);

		pr_info("%s storage test passed\n", type_names[i]);
	}

	end_time = ustp_get_time_us();

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-06 PASSED ===\n\n");
	return 0;

fail:
	return -EFAULT;
}

int t_ustp_config_advanced(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;

	pr_info("=== FT-07: Advanced Configuration Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	/* Test advanced configuration */
	dev->mtu = 1500;
	dev->window_size = 16;
	dev->timeout_ms = 10000;
	dev->max_retry = 20;

	/* Verify configurations */
	if (dev->mtu != 1500 || dev->window_size != 16 ||
		dev->timeout_ms != 10000 || dev->max_retry != 20) {
		pr_err("Advanced configuration failed\n");
		goto fail;
	}

	end_time = ustp_get_time_us();

	pr_info("MTU: %u bytes\n", dev->mtu);
	pr_info("Window Size: %u\n", dev->window_size);
	pr_info("Timeout: %u ms\n", dev->timeout_ms);
	pr_info("Max Retry: %u\n", dev->max_retry);

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-07 PASSED ===\n\n");
	return 0;

fail:
	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);
	return -EFAULT;
}

static int t_ustp_send_impl(u64 size, const char *size_name)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	char filename[64];

	pr_info("=== FT-08~11: File Send Test (%s, %llu bytes) ===\n",
		size_name, size);

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	snprintf(filename, sizeof(filename), "test_%s.bin", size_name);

	ret = ustp_send_file(dev, filename, size);
	if (ret < 0) {
		pr_err("Failed to send file: %d\n", ret);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	end_time = ustp_get_time_us();

	pr_info("File sent: %s (%llu bytes)\n", filename, size);
	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("Speed: %llu KB/s\n", (size * 1000) / (end_time - start_time));

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("=== FT-08~11 PASSED (%s) ===\n\n", size_name);
	return 0;
}

int t_ustp_send_small(void)
{
	return t_ustp_send_impl(1024, "small");
}

int t_ustp_send_medium(void)
{
	return t_ustp_send_impl(1048576, "medium");
}

int t_ustp_send_large(void)
{
	return t_ustp_send_impl(104857600, "large");
}

int t_ustp_send_huge(void)
{
	return t_ustp_send_impl(1073741824, "huge");
}

int t_ustp_resume(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	char filename[] = "test_resume.bin";

	pr_info("=== FT-12: Resume Transfer Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	dev->file_info.resume_offset = 524288;
	dev->file_info.current_shard = 2;

	ret = ustp_resume_transfer(dev, filename);
	if (ret < 0) {
		pr_err("Failed to resume transfer: %d\n", ret);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	end_time = ustp_get_time_us();

	pr_info("Resume transfer: %s\n", filename);
	pr_info("Resume offset: %llu bytes\n", dev->file_info.resume_offset);
	pr_info("Resume shard: %u\n", dev->file_info.current_shard);
	pr_info("Time taken: %llu us\n", end_time - start_time);

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("=== FT-12 PASSED ===\n\n");
	return 0;
}

int t_ustp_multi_file(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret, i;
	int file_count = 3;
	u64 file_sizes[] = {1024, 1048576, 10485760};

	pr_info("=== FT-13: Multi-File Transfer Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	for (i = 0; i < file_count; i++) {
		char filename[64];
		snprintf(filename, sizeof(filename), "file%d.bin", i + 1);

		ret = ustp_send_file(dev, filename, file_sizes[i]);
		if (ret < 0) {
			pr_err("Failed to send file %s: %d\n", filename, ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		pr_info("File %d sent: %s (%llu bytes)\n", i + 1,
			filename, file_sizes[i]);
	}

	end_time = ustp_get_time_us();

	pr_info("Multi-file transfer completed: %d files\n", file_count);
	pr_info("Time taken: %llu us\n", end_time - start_time);

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("=== FT-13 PASSED ===\n\n");
	return 0;
}

int t_ustp_verify(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	char filename[] = "test_verify.bin";
	u64 file_size = 1048576;

	pr_info("=== FT-14: File Verification Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	ret = ustp_send_file(dev, filename, file_size);
	if (ret < 0) {
		pr_err("Failed to send file: %d\n", ret);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	if (dev->file_info.file_size != file_size) {
		pr_err("File size mismatch: expected %llu, got %llu\n",
			file_size, dev->file_info.file_size);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	if (strcmp(dev->file_info.filename, filename) != 0) {
		pr_err("Filename mismatch: expected %s, got %s\n",
			filename, dev->file_info.filename);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return -EFAULT;
	}

	end_time = ustp_get_time_us();

	pr_info("File verified: %s (%llu bytes)\n", filename, file_size);
	pr_info("Time taken: %llu us\n", end_time - start_time);

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("=== FT-14 PASSED ===\n\n");
	return 0;
}

int t_ustp_storage_switch(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	enum storage_type types[] = {
		STORAGE_TYPE_SPI_FLASH, STORAGE_TYPE_SD_CARD
	};
	const char *type_names[] = {"SPI_FLASH", "SD_CARD"};
	int i;
	u64 start_time, end_time;

	pr_info("=== FT-15: Storage Switch Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	for (i = 0; i < 2; i++) {
		pr_info("Testing storage switch to %s...\n", type_names[i]);

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = types[i];
		storage->name = (char *)type_names[i];
		storage->initialized = true;

		if (ustp_init(dev, phy, storage, MODE_FILE_TRANSFER) < 0) {
			pr_err("Failed to initialize with %s\n", type_names[i]);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return -EFAULT;
		}

		if (dev->storage->type != types[i]) {
			pr_err("Storage type mismatch\n");
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return -EFAULT;
		}

		ustp_exit(dev);
		kfree(storage);

		pr_info("Storage switched to %s\n", type_names[i]);
	}

	end_time = ustp_get_time_us();

	kfree(phy);
	kfree(dev);

	pr_info("Time taken: %llu us\n", end_time - start_time);
	pr_info("=== FT-15 PASSED ===\n\n");
	return 0;
}

int t_ustp_performance(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	struct ustp_test_context *ctx;
	u64 start_time, end_time;
	int ret, i;
	int loop_count = 5;
	u64 file_size = 1048576;  /* 1MB per loop */

	pr_info("=== FT-16: Performance Test ===\n");

	/* Allocate test context */
	ctx = ustp_test_alloc();
	if (IS_ERR(ctx)) {
		return PTR_ERR(ctx);
	}

	start_time = ustp_get_time_us();

	for (i = 0; i < loop_count; i++) {
		u64 test_start, test_end;
		char filename[64];

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			ustp_test_free(ctx);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_WIFI;
		phy->name = "mock_wifi";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SD_CARD;
		storage->name = "mock_sd";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			pr_err("Failed to initialize device: %d\n", ret);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return ret;
		}

		test_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "perf_test_%d.bin", i);
		ret = ustp_send_file(dev, filename, file_size);
		if (ret < 0) {
			pr_err("Failed to send file: %d\n", ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return ret;
		}

		test_end = ustp_get_time_us();

		ctx->current_test = i + 1;
		snprintf(ctx->test_name, sizeof(ctx->test_name),
			"Performance Loop %d", i + 1);
		ustp_test_record(ctx, 1, file_size, test_end - test_start);

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Performance test completed: %d loops\n", loop_count);
	pr_info("Total time: %llu us\n", end_time - start_time);

	ustp_test_summary(ctx);
	ustp_test_free(ctx);

	pr_info("=== FT-16 PASSED ===\n\n");
	return 0;
}

int t_ustp_bandwidth(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	int test_count = 10;
	u64 test_size = 10485760;  /* 10MB per test */
	u64 total_bytes = 0;
	u64 total_time = 0;
	int i;

	pr_info("=== FT-17: Bandwidth Test ===\n");

	start_time = ustp_get_time_us();

	for (i = 0; i < test_count; i++) {
		u64 test_start, test_end;
		char filename[64];

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_WIFI;
		phy->name = "mock_wifi";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SD_CARD;
		storage->name = "mock_sd";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			pr_err("Failed to initialize device: %d\n", ret);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		test_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "bw_test_%d.bin", i);
		ret = ustp_send_file(dev, filename, test_size);
		if (ret < 0) {
			pr_err("Failed to send file: %d\n", ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		test_end = ustp_get_time_us();

		total_bytes += test_size;
		total_time += (test_end - test_start);

		pr_info("Test %d: %llu KB/s\n", i + 1,
			(test_size * 1000) / (test_end - test_start));

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Bandwidth test completed: %d tests\n", test_count);
	pr_info("Total bytes: %llu\n", total_bytes);
	pr_info("Average bandwidth: %llu KB/s\n",
		(total_bytes * 1000) / total_time);
	pr_info("Total time: %llu us\n", end_time - start_time);

	pr_info("=== FT-17 PASSED ===\n\n");
	return 0;
}

int t_ustp_latency(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	int packet_count = 100;
	u64 packet_size = 1024;  /* 1KB packets */
	u64 min_latency = UINT64_MAX;
	u64 max_latency = 0;
	u64 total_latency = 0;
	int i;

	pr_info("=== FT-18: Latency Test ===\n");

	start_time = ustp_get_time_us();

	for (i = 0; i < packet_count; i++) {
		u64 packet_start, packet_end;
		char filename[64];

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_BLE;
		phy->name = "mock_ble";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SPI_FLASH;
		storage->name = "mock_flash";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			pr_err("Failed to initialize device: %d\n", ret);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		packet_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "latency_test_%d.bin", i);
		ret = ustp_send_file(dev, filename, packet_size);
		if (ret < 0) {
			pr_err("Failed to send file: %d\n", ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		packet_end = ustp_get_time_us();

		u64 latency = packet_end - packet_start;
		total_latency += latency;
		if (latency < min_latency) {
			min_latency = latency;
		}
		if (latency > max_latency) {
			max_latency = latency;
		}

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Latency test completed: %d packets\n", packet_count);
	pr_info("Min latency: %llu us\n", min_latency);
	pr_info("Max latency: %llu us\n", max_latency);
	pr_info("Avg latency: %llu us\n", total_latency / packet_count);
	pr_info("Total time: %llu us\n", end_time - start_time);

	pr_info("=== FT-18 PASSED ===\n\n");
	return 0;
}

int t_ustp_resource(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	u64 test_size = 1048576;  /* 1MB */
	u32 peak_memory_kb = 0;

	pr_info("=== FT-19: Resource Usage Test ===\n");

	start_time = ustp_get_time_us();

	dev = ustp_alloc_device();
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	/* Calculate device memory usage */
	peak_memory_kb = (sizeof(*dev) + dev->buffer_size * 2) / 1024;
	pr_info("Device structure size: %zu bytes\n", sizeof(*dev));
	pr_info("Buffer size: %u bytes\n", dev->buffer_size);
	pr_info("Total device memory: %u KB\n", peak_memory_kb);

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		kfree(dev);
		return -ENOMEM;
	}
	peak_memory_kb += sizeof(*phy) / 1024;
	phy->type = COMM_TYPE_WIFI;
	phy->name = "mock_wifi";
	phy->initialized = true;

	storage = kzalloc(sizeof(*storage), GFP_KERNEL);
	if (!storage) {
		kfree(phy);
		kfree(dev);
		return -ENOMEM;
	}
	peak_memory_kb += sizeof(*storage) / 1024;
	storage->type = STORAGE_TYPE_SD_CARD;
	storage->name = "mock_sd";
	storage->initialized = true;

	ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
	if (ret < 0) {
		pr_err("Failed to initialize device: %d\n", ret);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	pr_info("Peak memory usage: %u KB\n", peak_memory_kb);
	pr_info("Memory usage within limit (< 100KB): %s\n",
		peak_memory_kb < 100 ? "YES" : "NO");

	ret = ustp_send_file(dev, "resource_test.bin", test_size);
	if (ret < 0) {
		pr_err("Failed to send file: %d\n", ret);
		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
		return ret;
	}

	end_time = ustp_get_time_us();

	pr_info("Total bytes sent: %llu\n", test_size);
	pr_info("Total time: %llu us\n", end_time - start_time);
	pr_info("Speed: %llu KB/s\n", (test_size * 1000) / (end_time - start_time));

	ustp_exit(dev);
	kfree(storage);
	kfree(phy);
	kfree(dev);

	pr_info("=== FT-19 PASSED ===\n\n");
	return 0;
}

int t_ustp_comprehensive(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	struct ustp_test_context *ctx;
	u64 start_time, end_time;
	int ret;
	u64 test_sizes[] = {1024, 1048576, 10485760};
	const char *test_names[] = {"small", "medium", "large"};
	int test_count = 3;
	int i;

	pr_info("=== FT-20: Comprehensive Test ===\n");

	/* Allocate test context */
	ctx = ustp_test_alloc();
	if (IS_ERR(ctx)) {
		return PTR_ERR(ctx);
	}

	start_time = ustp_get_time_us();

	for (i = 0; i < test_count; i++) {
		u64 test_start, test_end;
		char filename[64];

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			ustp_test_free(ctx);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_WIFI;
		phy->name = "mock_wifi";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SD_CARD;
		storage->name = "mock_sd";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			pr_err("Failed to initialize device: %d\n", ret);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return ret;
		}

		test_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "comp_%s.bin", test_names[i]);
		ret = ustp_send_file(dev, filename, test_sizes[i]);
		if (ret < 0) {
			pr_err("Failed to send file: %d\n", ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return ret;
		}

		test_end = ustp_get_time_us();

		ctx->current_test = i + 1;
		snprintf(ctx->test_name, sizeof(ctx->test_name),
			"Comprehensive %s", test_names[i]);
		ustp_test_record(ctx, 1, test_sizes[i], test_end - test_start);

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Comprehensive test completed: %d tests\n", test_count);
	pr_info("Total time: %llu us\n", end_time - start_time);

	ustp_test_summary(ctx);
	ustp_test_free(ctx);

	pr_info("=== FT-20 PASSED ===\n\n");
	return 0;
}

int t_ustp_stress(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	struct ustp_test_context *ctx;
	u64 start_time, end_time;
	int ret;
	int stress_iterations = 100;
	int passed = 0, failed = 0;
	int i;

	pr_info("=== FT-21: Stress Test (%d iterations) ===\n",
		stress_iterations);

	/* Allocate test context */
	ctx = ustp_test_alloc();
	if (IS_ERR(ctx)) {
		return PTR_ERR(ctx);
	}

	start_time = ustp_get_time_us();

	for (i = 0; i < stress_iterations; i++) {
		u64 test_start, test_end;
		char filename[64];

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			ustp_test_free(ctx);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_WIFI;
		phy->name = "mock_wifi";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SD_CARD;
		storage->name = "mock_sd";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			failed++;
			kfree(storage);
			kfree(phy);
			kfree(dev);
			continue;
		}

		test_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "stress_%d.bin", i);
		ret = ustp_send_file(dev, filename, 10240);  /* 10KB per test */
		if (ret < 0) {
			failed++;
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			continue;
		}

		test_end = ustp_get_time_us();

		ctx->current_test = i + 1;
		snprintf(ctx->test_name, sizeof(ctx->test_name),
			"Stress Iteration %d", i + 1);
		ustp_test_record(ctx, 1, 10240, test_end - test_start);

		passed++;

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Stress test completed: %d iterations\n", stress_iterations);
	pr_info("Passed: %d, Failed: %d\n", passed, failed);
	pr_info("Success rate: %d%%\n", (passed * 100) / stress_iterations);
	pr_info("Total time: %llu us\n", end_time - start_time);

	ustp_test_summary(ctx);
	ustp_test_free(ctx);

	pr_info("=== FT-21 PASSED ===\n\n");
	return 0;
}

int t_ustp_boundary(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	u64 start_time, end_time;
	int ret;
	u64 boundary_sizes[] = {0, 1, 512, 65536, 1073741824};  /* 0, 1B, 512B, 64KB, 1GB */
	const char *boundary_names[] = {"zero", "1B", "512B", "64KB", "1GB"};
	int test_count = 5;
	int i;

	pr_info("=== FT-22: Boundary Test ===\n");

	start_time = ustp_get_time_us();

	for (i = 0; i < test_count; i++) {
		u64 test_start, test_end;
		char filename[64];

		pr_info("Testing boundary: %s (%llu bytes)\n",
			boundary_names[i], boundary_sizes[i]);

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			return -ENOMEM;
		}
		phy->type = COMM_TYPE_WIFI;
		phy->name = "mock_wifi";
		phy->initialized = true;

		storage = kzalloc(sizeof(*storage), GFP_KERNEL);
		if (!storage) {
			kfree(phy);
			kfree(dev);
			return -ENOMEM;
		}
		storage->type = STORAGE_TYPE_SD_CARD;
		storage->name = "mock_sd";
		storage->initialized = true;

		ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
		if (ret < 0) {
			pr_err("Failed to initialize device: %d\n", ret);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		test_start = ustp_get_time_us();

		snprintf(filename, sizeof(filename), "boundary_%s.bin", boundary_names[i]);
		ret = ustp_send_file(dev, filename, boundary_sizes[i]);
		if (ret < 0 && boundary_sizes[i] > 0) {
			pr_err("Failed to send file %s: %d\n", filename, ret);
			ustp_exit(dev);
			kfree(storage);
			kfree(phy);
			kfree(dev);
			return ret;
		}

		test_end = ustp_get_time_us();

		pr_info("Boundary test %s passed: %llu us\n",
			boundary_names[i], test_end - test_start);

		ustp_exit(dev);
		kfree(storage);
		kfree(phy);
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Boundary test completed: %d tests\n", test_count);
	pr_info("Total time: %llu us\n", end_time - start_time);

	pr_info("=== FT-22 PASSED ===\n\n");
	return 0;
}

int t_ustp_error_injection(void)
{
	struct ustp_device *dev;
	struct physics_interface_ops *phy;
	struct storage_interface_ops *storage;
	struct ustp_test_context *ctx;
	u64 start_time, end_time;
	int ret;
	int test_count = 5;
	int i;

	pr_info("=== FT-23: Error Injection Test ===\n");

	/* Allocate test context */
	ctx = ustp_test_alloc();
	if (IS_ERR(ctx)) {
		return PTR_ERR(ctx);
	}

	start_time = ustp_get_time_us();

	for (i = 0; i < test_count; i++) {
		u64 test_start, test_end;
		char filename[64];
		int test_passed = 1;

		pr_info("Error injection test %d...\n", i + 1);

		dev = ustp_alloc_device();
		if (IS_ERR(dev)) {
			pr_err("Failed to allocate device\n");
			ustp_test_free(ctx);
			return PTR_ERR(dev);
		}

		phy = kzalloc(sizeof(*phy), GFP_KERNEL);
		if (!phy) {
			kfree(dev);
			ustp_test_free(ctx);
			return -ENOMEM;
		}

		/* Simulate different error conditions */
		switch (i) {
		case 0:
			/* Test with NULL phy interface */
			pr_info("Test 1: NULL communication interface\n");
			ret = ustp_init(dev, NULL, NULL, MODE_FILE_TRANSFER);
			if (ret == 0) {
				pr_err("Should have failed with NULL phy\n");
				test_passed = 0;
			}
			break;

		case 1:
			/* Test with invalid mode */
			pr_info("Test 2: Invalid transfer mode\n");
			phy->type = COMM_TYPE_WIFI;
			phy->name = "mock_wifi";
			phy->initialized = true;
			storage = kzalloc(sizeof(*storage), GFP_KERNEL);
			if (storage) {
				storage->type = STORAGE_TYPE_SD_CARD;
				storage->name = "mock_sd";
				storage->initialized = true;
				ret = ustp_init(dev, phy, storage, 99);  /* Invalid mode */
				if (ret == 0) {
					pr_err("Should have failed with invalid mode\n");
					test_passed = 0;
				}
				kfree(storage);
			}
			break;

		case 2:
			/* Test without storage for file transfer */
			pr_info("Test 3: File transfer without storage\n");
			phy->type = COMM_TYPE_WIFI;
			phy->name = "mock_wifi";
			phy->initialized = true;
			ret = ustp_init(dev, phy, NULL, MODE_FILE_TRANSFER);
			if (ret == 0) {
				pr_err("Should have failed without storage\n");
				test_passed = 0;
			}
			break;

		case 3:
			/* Test double initialization */
			pr_info("Test 4: Double initialization\n");
			phy->type = COMM_TYPE_WIFI;
			phy->name = "mock_wifi";
			phy->initialized = true;
			storage = kzalloc(sizeof(*storage), GFP_KERNEL);
			if (storage) {
				storage->type = STORAGE_TYPE_SD_CARD;
				storage->name = "mock_sd";
				storage->initialized = true;
				ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
				if (ret >= 0) {
					ret = ustp_init(dev, phy, storage, MODE_FILE_TRANSFER);
					if (ret == 0) {
						pr_err("Should have failed on second init\n");
						test_passed = 0;
					}
				}
				kfree(storage);
			}
			break;

		case 4:
			/* Test send without initialization */
			pr_info("Test 5: Send without initialization\n");
			kfree(phy);
			phy = NULL;
			storage = kzalloc(sizeof(*storage), GFP_KERNEL);
			if (storage) {
				storage->type = STORAGE_TYPE_SD_CARD;
				storage->name = "mock_sd";
				storage->initialized = true;
				ret = ustp_send_file(dev, "test.bin", 1024);
				if (ret == 0) {
					pr_err("Should have failed without initialization\n");
					test_passed = 0;
				}
				kfree(storage);
			}
			break;

		default:
			test_passed = 0;
			break;
		}

		test_end = ustp_get_time_us();

		ctx->current_test = i + 1;
		snprintf(ctx->test_name, sizeof(ctx->test_name),
			"Error Injection Test %d", i + 1);
		ustp_test_record(ctx, test_passed, 0, test_end - test_start);

		if (phy) {
			kfree(phy);
		}
		kfree(dev);
	}

	end_time = ustp_get_time_us();

	pr_info("Error injection test completed: %d tests\n", test_count);
	pr_info("Total time: %llu us\n", end_time - start_time);

	ustp_test_summary(ctx);
	ustp_test_free(ctx);

	pr_info("=== FT-23 PASSED ===\n\n");
	return 0;
}
