#ifndef __MHYDROLOGY_H
#define __MHYDROLOGY_H

#include <generated/deconfig.h>
#include <linux/types.h>
#include <linux/device.h>

#include <kinetis/fatfs-intf.h>
#include <kinetis/tim-task.h>

#define ELEMENT_COUNT                   298

#define ERC1                            1
#define ERC2                            2
#define ERC3                            3
#define ERC4                            4
#define ERC5                            5
#define ERC6                            6
#define ERC7                            7
#define ERC8                            8
#define ERC9                            9
#define ERC10                           10
#define ERC11                           11
#define ERC12                           12
#define ERC13                           13
#define ERC14                           14
#define ERC15                           15
#define ERC16                           16
#define ERC17                           17
#define ERC18                           18
#define ERC19                           19

#define SOH                             0x7E
#define STX                             0x02
#define SYN                             0x16
#define ETX                             0x03
#define ETB                             0x17
#define ENQ                             0x05
#define EOT                             0x04
#define ACK                             0x06
#define NAK                             0x15
#define ESC                             0x1B

/* SL651-2014水文规约帧结构定义 */
#define HYDROLOGY_FRAME_START_SOH       0x7E7E  /* 帧起始符 */
#define HYDROLOGY_FRAME_START_STX       0x02    /* 报文起始符 */
#define HYDROLOGY_FRAME_END_ETX         0x03    /* 报文结束符 */

enum hydrology_mode {
	HYDROLOGY_M1,
	HYDROLOGY_M2,
	HYDROLOGY_M3,
	HYDROLOGY_M4,
};

enum hydrology_rtu_type {
	RAINFALL = 0X50,
	RIVER_COURSE = 0X48,
	RESERVOIR = 0X4B,
	GATE_DAM = 0X5A,
	PUMPING_STATION = 0X44,
	TIDE = 0X54,
	SOIL_MOISTURE = 0X4D,
	GROUND_WATER = 0X47,
	WATER_QUALITY = 0X51,
	WATER_INTAKE = 0X49,
	OUTFALL = 0X4F,
};

static inline char *hydrology_type_rtu_string(enum hydrology_rtu_type type)
{
	switch (type) {
	case RAINFALL:
		return "rainfall";
	case RIVER_COURSE:
		return "lriver course";
	case RESERVOIR:
		return "reservoir";
	case GATE_DAM:
		return "gate_dam";
	case PUMPING_STATION:
		return "pumping station";
	case TIDE:
		return "tide";
	case SOIL_MOISTURE:
		return "soil moisture";
	case GROUND_WATER:
		return "ground water";
	case WATER_QUALITY:
		return "water quality";
	case WATER_INTAKE:
		return "water intake";
	case OUTFALL:
		return "outfall";
	default:
		return "unknown";
	}
}

enum hydrology_body_type {
	LINK_REPORT = 0x2F,			   // 遥测站链路维持报
	TEST_REPORT,				   // 遥测站测试报
	EVEN_PERIOD_INFO_REPORT,	   // 均匀时段水文信息报
	TIMER_REPORT,				   // 遥测站定时报
	ADD_REPORT,					   // 遥测站加报报
	HOUR_REPORT,				   // 遥测站小时报
	ARTIFICIAL_NUM_REPORT,		   // 遥测站人工置数报
	PICTURE_REPORT,				   // 遥测站图片报
	REAL_TIME_REPORT,			   // 中心站查询遥测站实时数据
	PERIOD_REPORT,				   // 中心站查询遥测站时段数据
	INQUIRE_ARTIFICIAL_NUM_REPORT, // 中心站查询遥测站人工置数
	SPECIFIED_ELEMENT_REPORT,	   // 中心站查询遥测站指定要素实时数据
	CONFIG_WRITE_REPORT = 0x40,	   // 遥测站配置修改
	CONFIG_READ_REPORT,			   // 遥测站配置读取
	PARA_WRITE_REPORT,			   // 中心站修改遥测站运行参数
	PARA_READ_REPORT,			   // 中心站读取遥测站运行参数
	WATER_PUMP_MOTOR_REPORT,	   // 中心站查询水泵电机实时工作数据
	SW_VERSION_REPORT,			   // 中心站查询遥测站查询遥测站软件版本
	STATUS_REPORT,				   // 中心站查询遥测站状态信息
	INIT_SOLID_STORAGE_REPORT,	   // 初始化固态存储数据
	RESET_REPORT,				   // 恢复遥测站出厂设置
	CHANGE_PASSWORD_REPORT,		   // 中心站修改传输密码
	SET_CLOCK_REPORT,			   // 中心站设置遥测站时钟
	SET_IC_CARD_REPORT,			   // 中心站设置遥测站IC卡状态
	PUMP_REPORT,				   // 中心站设置遥测站水泵开关命令响应/ 水泵状态自报
	VALVE_REPORT,				   // 中心站设置遥测站控制阀门开关命令响应/ 阀门状态自报
	GATE_REPORT,				   // 中心站设置遥测站控制闸门开关命令响应/ 闸门状态信息自报
	WATER_SETTING_REPORT,		   // 中心站设置遥测站水量定值控制命令响应
	RECORD_REPORT,				   // 中心站查询遥测站事件记录
	TIME_REPORT,				   // 中心站查询遥测站时钟
};

static inline char *hydrology_type_string(enum hydrology_body_type type)
{
	switch (type) {
	case LINK_REPORT:
		return "link report";

	case TEST_REPORT:
		return "test report";

	case EVEN_PERIOD_INFO_REPORT:
		return "even period info Report";

	case TIMER_REPORT:
		return "timer report";

	case ADD_REPORT:
		return "add report";

	case HOUR_REPORT:
		return "hour report";

	case ARTIFICIAL_NUM_REPORT:
		return "artificial num report";

	case PICTURE_REPORT:
		return "picture report";

	case REAL_TIME_REPORT:
		return "real time report";

	case PERIOD_REPORT:
		return "period report";

	case INQUIRE_ARTIFICIAL_NUM_REPORT:
		return "inquire artificial num report";

	case SPECIFIED_ELEMENT_REPORT:
		return "specified element report";

	case CONFIG_WRITE_REPORT:
		return "config write report";

	case CONFIG_READ_REPORT:
		return "config read report";

	case PARA_WRITE_REPORT:
		return "para write report";

	case PARA_READ_REPORT:
		return "para read report";

	case WATER_PUMP_MOTOR_REPORT:
		return "water pump motor report";

	case SW_VERSION_REPORT:
		return "sw version report";

	case STATUS_REPORT:
		return "status report";

	case INIT_SOLID_STORAGE_REPORT:
		return "init solid storage report";

	case RESET_REPORT:
		return "reset report";

	case CHANGE_PASSWORD_REPORT:
		return "change password report";

	case SET_CLOCK_REPORT:
		return "set clock report";

	case SET_IC_CARD_REPORT:
		return "set ic card report";

	case PUMP_REPORT:
		return "pump report";

	case VALVE_REPORT:
		return "valve report";

	case GATE_REPORT:
		return "gate report";

	case WATER_SETTING_REPORT:
		return "watersetting report";

	case RECORD_REPORT:
		return "record report";

	case TIME_REPORT:
		return "time report";

	default:
		return "unknown report";
	}
}

enum hydrology_msg_src_type {
	MSG_FORM_SERVER,
	MSG_FORM_CLIENT
};

//#pragma pack(1)

struct hydrology_element_info {
	u8 ID;
	u8 D;
	u8 d;
	u32 addr;
};

struct hydrology_element {
	u8 guide[2];
	u8 *value;
	u32 num;
};

//遥测站上行报文报头结构
struct hydrology_up_header {
	u8 frame_start[2];
	u8 center_addr;
	u8 remote_addr[5];
	u8 password[2];
	u8 funcode;
	u8 dir_len[2];
	u8 paket_start;
	u8 count_seq[3];
	u8 len;
};

//遥测站下行报文报头结构
struct hydrology_down_header {
	u8 frame_start[2];
	u8 remote_addr[5];
	u8 center_addr;
	u8 password[2];
	u8 funcode;
	u8 dir_len[2];
	u8 paket_start;
	u8 count_seq[3];
	u8 len;
};

//遥测站上行报文正文结构
struct hydrology_up_body {
	u8 stream_id[2];
	u8 send_time[6];
	u8 rtu_addr_id[2];
	u8 rtu_addr[5];
	u8 rtu_type;
	u8 observationtimeid[2];
	u8 observation_time[5];
	u16 count;
	struct hydrology_element **element;
	u16 len;
};

//遥测站下行报文正文结构
struct hydrology_down_body {
	u8 stream_id[2];
	u8 send_time[6];
	u8 rtu_addr_id[2];
	u8 rtu_addr[5];
	u16 count;
	struct hydrology_element **element;
	u16 len;
};

struct hydrology_packet {
	void *header;
	void *body;
	u8 end;
	u16 crc16;
	u8 *buffer;
	u16 len;
};

struct hydrology {
	struct hydrology_packet *up_packet;
	struct hydrology_packet *down_packet;
	unsigned source: 1;

	struct device dev;

	struct tim_task collecte_data;
	struct tim_task link_pkt;
};
#define	to_hydrology(d) container_of(d, struct hydrology, dev)

//#pragma pack()

void hydrology_get_time(u8 *time);
void hydrology_set_time(u8 *time);
int hydrology_open_port(void);
int hydrology_close_port(void);
int hydrology_port_transmmit(u8 *pdata, u16 length);
int hydrology_port_receive(u8 **ppdata, u16 *plength, u32 Timeout);
void hydrology_get_observation_time(struct hydrology_element_info *element, u8 *observation_time);
void hydrology_set_observation_time(struct hydrology_element_info *element);
void hydrology_get_guide_id(u8 *value, u8 D, u8 d);
int hydrology_convert_to_hex_element(double input, int D, int d, u8 *out);
int hydrology_malloc_element(u8 guide, u8 D, u8 d,
	struct hydrology_element *element);
void hydrology_free_element(struct hydrology_element *element);
void hydrology_get_stream_id(u8 *stream_id);
u16 hydrology_calculate_crc16(const u8 *data, u16 length);
bool hydrology_verify_crc16(const u8 *data, u16 length);
int hydrology_device_process_send(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_mode mode, enum hydrology_body_type funcode);
int hydrology_read_specified_element_info(struct hydrology_element_info *element,
	enum hydrology_body_type funcode, u16 index);
int hydrology_host_process_send(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_mode mode, enum hydrology_body_type funcode, u8 end);
int hydrology_device_process(struct hydrology_element_info *element_table, u8 cnt,
	enum hydrology_mode mode, enum hydrology_body_type funcode);
void hydrology_disable_link_packet(void);
void hydrology_enable_link_packet(void);
void hydrology_disconnect_link(void);
int hydrology_device_reboot(void);

extern struct hydrology g_hydrology;

#endif
