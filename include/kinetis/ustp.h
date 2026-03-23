/**
 * @file ustp.h
 * @brief Universal Serial Transfer Protocol (USTP) Header File
 * @version 3.0
 * @date 2026-03-22
 *
 * USTP is a generic serial transfer protocol that supports:
 * - Large file transfer (GB level)
 * - Streaming data transfer
 * - Abstract communication interface (BLE/WiFi/UART)
 * - Abstract storage interface (FATFS/Raw Flash)
 * - Reliable transfer with sliding window
 * - Resume from breakpoint
 */

#ifndef _KINETIS_USTP_H
#define _KINETIS_USTP_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <pthread.h>

/*********************************************************************
 * Protocol Version
 *********************************************************************/

#define PROTOCOL_VERSION_MAJOR    0x03
#define PROTOCOL_VERSION_MINOR    0x00
#define PROTOCOL_VERSION          ((PROTOCOL_VERSION_MAJOR << 8) | PROTOCOL_VERSION_MINOR)

/*********************************************************************
 * Protocol Constants
 *********************************************************************/

/* Synchronization bytes */
#define USTP_SYNC_BYTE1           0xAA
#define USTP_SYNC_BYTE2           0x55
#define USTP_SYNC_WORD            0xAA55

/* Frame header size */
#define USTP_HEADER_SIZE          20

/* Default transfer parameters */
#define DEFAULT_PACKET_SIZE       1024
#define DEFAULT_WINDOW_SIZE       8
#define DEFAULT_TIMEOUT_MS        3000
#define MAX_RETRY_COUNT           5
#define HEARTBEAT_INTERVAL_MS     5000
#define MAX_FILE_SIZE             (1ULL << 40)  /* Max 1TB */

/* Flash related */
#define FLASH_SECTOR_SIZE         4096
#define FLASH_BLOCK_SIZE          65536
#define FLASH_WRITE_BUFFER_SIZE   4096
#define DEFAULT_SHARD_SIZE        65536

/*********************************************************************
 * Transfer Mode
 *********************************************************************/

/**
 * @brief USTP transfer mode
 */
enum ustp_mode {
    MODE_FILE_TRANSFER  = 0x01,  /* File transfer mode (requires storage layer) */
    MODE_STREAMING      = 0x02,  /* Streaming transfer mode (optional storage layer) */
};

/*********************************************************************
 * Communication Interface Type
 *********************************************************************/

/**
 * @brief Communication interface type
 */
enum comm_interface_type {
    COMM_TYPE_HC05      = 0x01,  /* HC-05 Bluetooth module */
    COMM_TYPE_WIFI      = 0x02,  /* WiFi module */
    COMM_TYPE_UART      = 0x03,  /* UART direct connection */
    COMM_TYPE_NB_IOT    = 0x04,  /* NB-IoT module */
    COMM_TYPE_4G        = 0x05,  /* 4G module */
    COMM_TYPE_MAX
};

/*********************************************************************
 * Storage Interface Type
 *********************************************************************/

/**
 * @brief Storage interface type
 */
enum storage_type {
    STORAGE_TYPE_SPI_FLASH  = 0x01,  /* SPI Flash (W25Qxxx) */
    STORAGE_TYPE_QSPI_FLASH = 0x02,  /* QSPI Flash */
    STORAGE_TYPE_SD_CARD    = 0x03,  /* SD Card */
    STORAGE_TYPE_EEPROM     = 0x04,  /* EEPROM */
    STORAGE_TYPE_EMMC       = 0x05,  /* eMMC */
    STORAGE_TYPE_MAX
};

/*********************************************************************
 * Transfer State
 *********************************************************************/

/**
 * @brief Transfer state machine
 */
enum transfer_state {
    STATE_IDLE,           /* Idle state */
    STATE_HANDSHAKE,      /* Handshaking */
    STATE_READY,          /* Ready to transfer */
    STATE_TRANSFERRING,   /* Transferring data */
    STATE_PAUSED,         /* Paused */
    STATE_COMPLETING,     /* Completing transfer */
    STATE_ABORTING,       /* Aborting transfer */
    STATE_ERROR           /* Error state */
};

/*********************************************************************
 * Command Type
 *********************************************************************/

/**
 * @brief USTP command types
 */
enum ustp_cmd {
    /* Handshake commands */
    CMD_HANDSHAKE       = 0x01,  /* Handshake request */
    CMD_HANDSHAKE_ACK   = 0x02,  /* Handshake response */
    CMD_PING            = 0x03,  /* Heartbeat ping */
    CMD_PONG            = 0x04,  /* Heartbeat response */

    /* Connection management */
    CMD_CONNECT         = 0x10,  /* Connection request */
    CMD_CONNECT_ACK     = 0x11,  /* Connection response */
    CMD_DISCONNECT      = 0x12,  /* Disconnect request */
    CMD_DISCONNECT_ACK  = 0x13,  /* Disconnect response */

    /* File management */
    CMD_FILE_INFO       = 0x20,  /* File information */
    CMD_FILE_INFO_ACK   = 0x21,  /* File information acknowledgment */
    CMD_FILE_START      = 0x22,  /* Start transfer */
    CMD_FILE_START_ACK  = 0x23,  /* Start transfer acknowledgment */
    CMD_FILE_END        = 0x24,  /* End of file */
    CMD_FILE_END_ACK    = 0x25,  /* End of file acknowledgment */
    CMD_FILE_ABORT      = 0x26,  /* Abort transfer */
    CMD_FILE_DELETE     = 0x27,  /* Delete file */
    CMD_FILE_QUERY      = 0x28,  /* Query file */

    /* Data block management (generic, supports file and streaming) */
    CMD_DATA_REQUEST    = 0x30,  /* Data block request */
    CMD_DATA            = 0x31,  /* Data block */
    CMD_DATA_ACK        = 0x32,  /* Data block acknowledgment */
    CMD_DATA_NAK        = 0x33,  /* Data block negative acknowledgment */

    /* Resume from breakpoint */
    CMD_RESUME_REQUEST  = 0x40,  /* Resume request */
    CMD_RESUME_RESPONSE = 0x41,  /* Resume response */
    CMD_QUERY_PROGRESS  = 0x42,  /* Query progress */

    /* Flow control */
    CMD_PAUSE           = 0x50,  /* Pause transfer */
    CMD_PAUSE_ACK       = 0x51,  /* Pause acknowledgment */
    CMD_RESUME          = 0x52,  /* Resume transfer */
    CMD_RESUME_ACK      = 0x53,  /* Resume acknowledgment */

    /* Configuration management */
    CMD_GET_CONFIG      = 0x60,  /* Get configuration */
    CMD_SET_CONFIG      = 0x61,  /* Set configuration */
    CMD_CONFIG_ACK      = 0x62,  /* Configuration acknowledgment */

    /* Error report */
    CMD_ERROR_REPORT    = 0xF0,  /* Error report */
    CMD_ERROR_ACK       = 0xF1,  /* Error acknowledgment */
};

/*********************************************************************
 * Error Code
 *********************************************************************/

/**
 * @brief Transfer error codes
 */
enum transfer_error {
    ERR_NONE             = 0x00,
    ERR_INVALID_HEADER   = 0x01,  /* Invalid frame header */
    ERR_CHECKSUM         = 0x02,  /* Checksum error */
    ERR_TIMEOUT          = 0x03,  /* Timeout */
    ERR_NO_SPACE         = 0x04,  /* Storage space insufficient */
    ERR_FILE_NOT_FOUND   = 0x05,  /* File not found */
    ERR_WRITE_FAILED     = 0x06,  /* Write failed */
    ERR_READ_FAILED      = 0x07,  /* Read failed */
    ERR_SEQ_ERROR        = 0x08,  /* Sequence number error */
    ERR_ABORTED          = 0x09,  /* Transfer aborted */
    ERR_PROTOCOL_VERSION = 0x0A,  /* Protocol version mismatch */
    ERR_NOT_SUPPORTED    = 0x0B,  /* Operation not supported */
    ERR_INVALID_PARAM    = 0x0C,  /* Invalid parameter */
    ERR_IO_ERROR         = 0x0D,  /* IO error */
    ERR_FLASH_ERROR      = 0x0E,  /* Flash error */
    ERR_STORAGE_ERROR    = 0x0F,  /* Storage layer error */
    ERR_COMM_ERROR       = 0x10,  /* Communication error */
    ERR_UNKNOWN          = 0xFF   /* Unknown error */
};

/*********************************************************************
 * Flag Definitions
 *********************************************************************/

/* Frame flags */
#define FLAG_FIRST_PKT     (1 << 0)  /* First packet */
#define FLAG_LAST_PKT      (1 << 1)  /* Last packet */
#define FLAG_NEED_ACK      (1 << 2)  /* Need acknowledgment */
#define FLAG_RETRANSMIT    (1 << 3)  /* Retransmit packet */
#define FLAG_COMPRESS      (1 << 4)  /* Compressed data */
#define FLAG_ENCRYPT       (1 << 5)  /* Encrypted data */
#define FLAG_CRC32         (1 << 6)  /* Use CRC32 */
#define FLAG_RESERVED      (1 << 7)  /* Reserved */

/*********************************************************************
 * Error Severity
 *********************************************************************/

/**
 * @brief Error severity levels
 */
enum error_severity {
    SEVERITY_INFO      = 0x00,  /* Information */
    SEVERITY_WARNING   = 0x01,  /* Warning */
    SEVERITY_ERROR     = 0x02,  /* Error */
    SEVERITY_CRITICAL  = 0x03   /* Critical error */
};

/*********************************************************************
 * Error Recovery Strategy
 *********************************************************************/

/**
 * @brief Error recovery strategies
 */
enum error_recovery_strategy {
    RECOVERY_RETRY,        /* Retry */
    RECOVERY_RESUME,       /* Resume from breakpoint */
    RECOVERY_ABORT,        /* Abort transfer */
    RECOVERY_IGNORE        /* Ignore error */
};

/*********************************************************************
 * Frame Header Structure
 *********************************************************************/

/**
 * @brief USTP frame header structure
 * @note Total size: 20 bytes
 */
struct ustp_header {
    /* Synchronization bytes */
    u8  sync[2];           /* Sync bytes 0xAA 0x55 */

    /* Protocol information */
    u8  version_major;     /* Protocol major version */
    u8  version_minor;     /* Protocol minor version */
    u8  cmd;               /* Command type */
    u8  flags;             /* Flags */
    u16 seq;               /* Sequence number (big-endian) */
    u16 length;            /* Data length (big-endian) */
    u16 checksum;          /* Checksum (CRC16) */

    /* Session information */
    u32 session_id;        /* Session ID */

    /* Reserved fields */
    u8  reserved[4];       /* Reserved for extension */
} __attribute__((packed));

/*********************************************************************
 * Payload Structures
 *********************************************************************/

/**
 * @brief Handshake payload
 */
struct handshake_payload {
    u8  protocol_version_major;
    u8  protocol_version_minor;
    u16 mtu;               /* Maximum transmission unit */
    u32 window_size;       /* Sliding window size */
    u32 capabilities;      /* Capability flags */
    u32 session_id;        /* Session ID */
} __attribute__((packed));

/**
 * @brief File information payload
 */
struct file_info_payload {
    u64 file_size;         /* File size (64-bit) */
    u32 shard_size;        /* Shard size */
    u32 total_shards;      /* Total shards */
    u16 file_name_len;     /* File name length */
    u8  file_name[256];    /* File name */
    u32 file_crc32;        /* File CRC32 */
    u8  flags;             /* File flags */
    u8  reserved[3];
} __attribute__((packed));

/**
 * @brief Data block payload (generic)
 */
struct data_payload {
    u32 block_id;          /* Data block ID */
    u32 block_index;       /* Data block index */
    u64 block_offset;      /* Data block offset */
    u32 block_size;        /* Data block size */
    u32 block_crc32;       /* Data block CRC32 */
    u8  data[0];           /* Data block data (variable length) */
} __attribute__((packed));

/**
 * @brief Resume request payload
 */
struct resume_request_payload {
    char filename[256];    /* File name */
    u64 resume_offset;     /* Resume offset */
    u32 resume_shard;      /* Resume shard */
} __attribute__((packed));

/**
 * @brief Resume response payload
 */
struct resume_response_payload {
    u32 session_id;        /* Session ID */
    u32 resume_shard;      /* Resume shard */
    u64 resume_offset;     /* Resume offset */
    u8  status;            /* Status: 0=success, others=failure */
    u8  reserved[3];
} __attribute__((packed));

/**
 * @brief Error report payload
 */
struct error_report_payload {
    u8  error_code;        /* Error code */
    u8  severity;          /* Severity level */
    u16 error_seq;         /* Error sequence number */
    u32 timestamp;         /* Timestamp */
    u32 context;           /* Context information */
    u8  message[64];       /* Error message */
} __attribute__((packed));

/**
 * @brief Data NAK payload
 */
struct data_nak_payload {
    u32 block_id;          /* Data block ID */
    u32 block_index;       /* Data block index */
    u8  error_code;        /* Error code */
    u8  retry_count;       /* Retry count */
    u8  reserved[2];
} __attribute__((packed));

struct physics_interface_ops {
    /* Device management */
    int (*init)(void *private);
    int (*deinit)(void *private);
    int (*reset)(void *private);

    /* Connection management */
    int (*connect)(void *private, const char *target);
    int (*disconnect)(void *private);
    int (*is_connected)(void *private);

    /* Data transfer */
    int (*send)(void *private, const u8 *data, u32 len, u32 timeout_ms);
    int (*recv)(void *private, u8 *data, u32 len, u32 timeout_ms);
    int (*send_async)(void *private, const u8 *data, u32 len);
    int (*recv_async)(void *private, u8 *data, u32 len);
    int (*flush)(void *private);

    /* Property query */
    u32 (*get_mtu)(void *private);
    int (*get_signal_strength)(void *private);
    int (*get_error_status)(void *private);

    /* Configuration management */
    int (*set_baudrate)(void *private, u32 baudrate);
    int (*get_baudrate)(void *private);
};

struct storage_properties {
    /* Capacity information */
    u64 total_size;         /* Total size (bytes) */
    u64 free_size;          /* Free size (bytes) */

    /* Block information */
    u32 block_size;         /* Block size (erase unit) */
    u32 page_size;          /* Page size (write unit) */
    u32 sector_size;        /* Sector size */

    /* File system */
    u32 max_file_size;      /* Maximum file size */
    u32 max_files;          /* Maximum number of files */
    u32 max_filename_len;   /* Maximum filename length */

    /* Feature flags */
    bool supports_erase;    /* Supports erase */
    bool supports_wear_leveling;  /* Supports wear leveling */
    bool supports_bad_block_management;  /* Supports bad block management */
    bool supports_fragmentation;  /* Supports defragmentation */
    bool is_volatile;       /* Is volatile storage */

    /* Performance parameters */
    u32 read_speed_kbps;    /* Read speed */
    u32 write_speed_kbps;   /* Write speed */
    u32 erase_time_ms;      /* Erase time */
};

/**
 * @brief File shard structure
 */
struct file_shard {
    u32 shard_id;           /* Shard ID (unique) */
    u32 shard_index;        /* Shard index (starts from 0) */
    u32 total_shards;       /* Total shards */

    /* File location information */
    u64 shard_offset;       /* Shard offset in original file */
    u32 shard_size;         /* Shard size */
    u32 shard_crc32;        /* Shard CRC32 */

    /* Flash storage location */
    u64 flash_addr;         /* Flash start address */
    u32 flash_size;         /* Flash occupied size */
    u32 flash_block_index;  /* Flash block index */

    /* Status */
    u8  status;             /* Status: 0=idle, 1=writing, 2=completed */
    u8  reserved[3];
};

/**
 * @brief File metadata structure
 */
struct file_metadata {
    /* File basic information */
    char filename[256];     /* File name */
    u64 file_size;          /* File size (64-bit, supports large files) */
    u32 file_crc32;         /* File overall CRC32 */

    /* Timestamps */
    u64 created_time;       /* Creation time (Unix timestamp) */
    u64 modified_time;      /* Modification time */
    u64 accessed_time;      /* Access time */

    /* Shard information */
    u32 total_shards;       /* Total shards */
    u32 completed_shards;   /* Completed shards */
    u32 shard_size;         /* Standard shard size */

    /* Storage location */
    u32 shard_table_addr;   /* Shard table Flash address */
    u32 shard_table_size;   /* Shard table size */

    /* File flags */
    u8  flags;              /* Flags */
    u8  reserved[3];

    /* Extended information */
    u32 checksum;           /* Metadata checksum */
};

/**
 * @brief Storage interface operations
 */
struct storage_interface_ops {
    /* Device management */
    int (*init)(void *private);
    int (*deinit)(void *private);
    int (*reset)(void *private);
    int (*get_properties)(void *private, struct storage_properties *props);
    int (*format)(void *private);

    /* Block operations */
    int (*erase)(void *private, u64 offset, u32 size);
    int (*read)(void *private, u64 offset, u8 *data, u32 len);
    int (*write)(void *private, u64 offset, const u8 *data, u32 len);

    /* File operations */
    int (*file_create)(void *private, const char *filename, u64 size,
                       struct file_metadata *meta);
    int (*file_delete)(void *private, const char *filename);
    int (*file_read)(void *private, const char *filename, u64 offset,
                     u8 *data, u32 len);
    int (*file_write)(void *private, const char *filename, u64 offset,
                      const u8 *data, u32 len);
    int (*file_get_metadata)(void *private, const char *filename,
                             struct file_metadata *meta);
    int (*file_list)(void *private, char *buffer, u32 len);

    /* Shard operations */
    int (*shard_create)(void *private, const struct file_shard *shard);
    int (*shard_delete)(void *private, u32 shard_id);
    int (*shard_read)(void *private, u32 shard_id, u8 *data, u32 len);
    int (*shard_write)(void *private, u32 shard_id, const u8 *data, u32 len);
    int (*shard_get_info)(void *private, u32 shard_id,
                          struct file_shard *shard);
    int (*shard_verify)(void *private, u32 shard_id);

    /* Metadata management */
    int (*metadata_save)(void *private, const char *filename,
                         const struct file_metadata *meta);
    int (*metadata_load)(void *private, const char *filename,
                         struct file_metadata *meta);
    int (*metadata_delete)(void *private, const char *filename);

    /* Defragmentation */
    int (*defragment)(void *private);

    /* Bad block management */
    int (*check_bad_block)(void *private, u64 offset);
    int (*mark_bad_block)(void *private, u64 offset);
};

struct packet_buffer {
    u16 seq;
    u8 *data;
    u32 len;
    bool acked;
    u32 timestamp;
    u8 retry_count;
};

/**
 * @brief Sliding window structure
 */
struct sliding_window {
    u16 window_size;       /* Window size */
    u16 send_base;         /* Send window base sequence */
    u16 next_seq;          /* Next sequence number */
    u16 last_ack;          /* Last acknowledgment sequence */

    /* Pending acknowledgment packets */
    struct packet_buffer packets[32];

    /* Statistics */
    u32 total_sent;
    u32 total_acked;
    u32 total_retransmitted;
};

/*********************************************************************
 * File Information V2 Structure
 *********************************************************************/

/**
 * @brief File information structure (V2)
 */
struct file_info_v2 {
    char filename[256];     /* File name */
    u64 file_size;          /* File size (64-bit) */
    u32 shard_size;         /* Shard size */
    u32 total_shards;       /* Total shards */
    u32 file_crc32;         /* File CRC32 */

    /* Resume from breakpoint */
    u64 resume_offset;      /* Resume offset */
    u32 current_shard;      /* Current shard */

    /* Metadata */
    u64 created_time;       /* Creation time */
    u64 modified_time;      /* Modification time */
    u8  flags;              /* File flags */
    u8  reserved[3];
};

/*********************************************************************
 * USTP Device Structure
 *********************************************************************/

/**
 * @brief USTP device structure
 * @note Uses abstract comm interface, NOT direct device dependency
 */
struct ustp_device {
    /* Communication interface (ABSTRACT, not device specific) */
    struct physics_interface_ops *comm;

    /* Storage interface (ABSTRACT, optional for pure streaming) */
    struct storage_interface_ops *storage;

    /* Transfer session */
    enum ustp_mode mode;  /* MODE_FILE_TRANSFER or MODE_STREAMING */
    u32 session_id;
    enum transfer_state state;

    /* For file transfer mode */
    struct file_info_v2 file_info;

    /* Progress tracking */
    u64 transferred_bytes;
    u64 current_shard_offset;
    u32 completed_shards;
    u32 total_shards;

    /* Sliding windows */
    struct sliding_window tx_window;
    struct sliding_window rx_window;

    /* Sequence number management */
    u16 tx_seq;
    u16 rx_seq;

    /* Threads */
    pthread_t rx_thread;
    pthread_t tx_thread;

    /* Status */
    bool initialized;
    bool transferring;
    bool paused;

    /* Thread safety */
    pthread_mutex_t lock;
    pthread_cond_t cond;

    /* Configuration */
    u32 mtu;
    u32 window_size;
    u32 timeout_ms;
    u8  max_retry;

    /* Statistics */
    u64 total_sent_bytes;
    u64 total_recv_bytes;
    u32 total_retransmits;
    u32 total_timeouts;

    /* Buffers */
    u8 *tx_buffer;
    u8 *rx_buffer;
    u32 buffer_size;

    /* Error code */
    int last_error;
};

#endif /* _KINETIS_USTP_H */
