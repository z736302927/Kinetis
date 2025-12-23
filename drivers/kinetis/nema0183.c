
#include <generated/deconfig.h>
#include <linux/err.h>
#include <linux/ktime.h>

struct ubx_protocol {
	u16 sync_char;
#define UBX_HEAD	0xB562

	u8 class;
#define UBX_CLASS_NAV	0x01
#define UBX_CLASS_RXM	0x02
#define UBX_CLASS_INF	0x04
#define UBX_CLASS_ACK	0x05
#define UBX_CLASS_CFG	0x06
#define UBX_CLASS_UPD	0x09
#define UBX_CLASS_MON	0x0A
#define UBX_CLASS_AID	0x0B
#define UBX_CLASS_TIM	0x0D
#define UBX_CLASS_ESF	0x10
#define UBX_CLASS_MGA	0x13
#define UBX_CLASS_LOG	0x21
#define UBX_CLASS_SEC	0x27
#define UBX_CLASS_HNR	0x28

	u8 id;
	/* UBX Class ACK Ack/Nak Messages */
#define UBX_CLASS_ACK_ACK            0x01 /* 2 Output Message acknowledged */
#define UBX_CLASS_ACK_NAK            0x00 /* 2 Output Message not acknowledged */

	/* UBX Class AID AssistNow Aiding Messages */
	/* 0 Poll Request Poll GPS aiding almanac data
	 * 1 Poll Request Poll GPS aiding almanac data for a SV
	 * (8) or (40) Input/Output GPS aiding almanac input/output...
	 */
#define UBX_CLASS_AID_ALM            0x30
	/* 0 Poll Request Poll AssistNow Autonomous data, all...
	 * 1 Poll Request Poll AssistNow Autonomous data, one...
	 * 68 Input/Output AssistNow Autonomous data
	 */
#define UBX_CLASS_AID_AOP            0x33
	/* 0 Poll Request Poll GPS aiding ephemeris data
	 * 1 Poll Request Poll GPS aiding ephemeris data for a SV
	 * (8) or (104) Input/Output GPS aiding ephemeris input/output...
	 */
#define UBX_CLASS_AID_EPH            0x31
	/* 0 Poll Request Poll GPS health, UTC, ionosphere...
	 * 72 Input/Output GPS health, UTC and ionosphere...
	 */
#define UBX_CLASS_AID_HUI            0x02
	/* 0 Poll Request Poll GPS initial aiding data
	 * 48 Input/Output Aiding position, time, frequency, clock...
	 */
#define UBX_CLASS_AID_INI            0x01

	/* UBX Class CFG Configuration Input Messages */
#define UBX_CLASS_CFG_ANT            0x13 /* 4 Get/set Antenna control settings */
#define UBX_CLASS_CFG_BATCH          0x93 /* 8 Get/set Get/set data batching configuration */
#define UBX_CLASS_CFG_CFG            0x09 /* (12) or (13) Command Clear, save and load configurations */
	/* 44 Set Set user-defined datum
	 * 52 Get Get currently defined datum
	 */
#define UBX_CLASS_CFG_DAT            0x06
#define UBX_CLASS_CFG_DGNSS          0x70 /* 4 Get/set DGNSS configuration */
#define UBX_CLASS_CFG_DOSC           0x61 /* 4 + 32*numO... Get/set Disciplined oscillator configuration */
#define UBX_CLASS_CFG_ESFALG         0x56 /* 12 Get/set Get/set IMU-mount misalignment... */
#define UBX_CLASS_CFG_ESFA           0x4C /* 20 Get/set Get/set the Accelerometer (A) sensor... */
#define UBX_CLASS_CFG_ESFG           0x4D /* 20 Get/set Get/set the Gyroscope (G) sensor... */
#define UBX_CLASS_CFG_ESFWT          0x82 /* 32 Get/set Get/set wheel-tick configuration */
#define UBX_CLASS_CFG_ESRC           0x60 /* 4 + 36*numS... Get/set External synchronization source... */
#define UBX_CLASS_CFG_GEOFENCE       0x69 /* 8 + 12*numF... Get/set Geofencing configuration */
#define UBX_CLASS_CFG_GNSS           0x3E /* 4 + 8*numCo... Get/set GNSS system configuration */
#define UBX_CLASS_CFG_HNR            0x5C /* 4 Get/set High navigation rate settings */
	/* 1 Poll Request Poll configuration for one protocol
	 * 0 + 10*N Get/set Information message configuration
	 */
#define UBX_CLASS_CFG_INF            0x02
#define UBX_CLASS_CFG_ITFM           0x39 /* 8 Get/set Jamming/interference monitor... */
#define UBX_CLASS_CFG_LOGFILTER      0x47 /* 12 Get/set Data logger configuration */
	/* 2 Poll Request Poll a message configuration
	 * 8 Get/set Set message rate(s)
	 * 3 Get/set Set message rate
	 */
#define UBX_CLASS_CFG_MSG            0x01
#define UBX_CLASS_CFG_NAV5           0x24 /* 36 Get/set Navigation engine settings */
	/* 40 Get/set Navigation engine expert settings
	 * 40 Get/set Navigation engine expert settings
	 * 44 Get/set Navigation engine expert settings
	 */
#define UBX_CLASS_CFG_NAVX5          0x23
	/* 4 Get/set NMEA protocol configuration...
	 * 12 Get/set NMEA protocol configuration V0...
	 * 20 Get/set Extended NMEA protocol configuration V1
	 */
#define UBX_CLASS_CFG_NMEA           0x17
#define UBX_CLASS_CFG_ODO            0x1E /* 20 Get/set Odometer, low-speed COG engine... */
	/* 44 Get/set Extended power management...
	 * 48 Get/set Extended power management...
	 * 48 Get/set Extended power management...
	 */
#define UBX_CLASS_CFG_PM2            0x3B
#define UBX_CLASS_CFG_PMS            0x86 /* 8 Get/set Power mode setup */
	/* 1 Poll Request Polls the configuration for one I/O port
	 * 20 Get/set Port configuration for UART ports
	 * 20 Get/set Port configuration for USB port
	 * 20 Get/set Port configuration for SPI port
	 * 20 Get/set Port configuration for I2C (DDC) port
	 */
#define UBX_CLASS_CFG_PRT            0x00
#define UBX_CLASS_CFG_PWR            0x57 /* 8 Set Put receiver in a defined power state */
#define UBX_CLASS_CFG_RATE           0x08 /* 6 Get/set Navigation/measurement rate settings */
#define UBX_CLASS_CFG_RINV           0x34 /* 1 + 1*N Get/set Contents of remote inventory */
#define UBX_CLASS_CFG_RST            0x04 /* 4 Command Reset receiver / Clear backup data... */
	/* 2 Get/set RXM configuration
	 * 2 Get/set RXM configuration
	 */
#define UBX_CLASS_CFG_RXM            0x11
#define UBX_CLASS_CFG_SBAS           0x16 /* 8 Get/set SBAS configuration */
#define UBX_CLASS_CFG_SENIF          0x88 /* 6 Get/set I2C sensor interface configuration */
#define UBX_CLASS_CFG_SLAS           0x8D /* 4 Get/set SLAS configuration */
#define UBX_CLASS_CFG_SMGR           0x62 /* 20 Get/set Synchronization manager configuration */
#define UBX_CLASS_CFG_SPT            0x64 /* 12 Get/set Configure and start a sensor... */
#define UBX_CLASS_CFG_TMODE2         0x3D /* 28 Get/set Time mode settings 2 */
#define UBX_CLASS_CFG_TMODE3         0x71 /* 40 Get/set Time mode settings 3 */
	/* 0 Poll Request Poll time pulse parameters for time...
	 * 1 Poll Request Poll time pulse parameters
	 * 32 Get/set Time pulse parameters
	 * 32 Get/set Time pulse parameters
	 */
#define UBX_CLASS_CFG_TP5            0x31
#define UBX_CLASS_CFG_TXSLOT         0x53 /* 16 Set TX buffer time slots configuration */
#define UBX_CLASS_CFG_USB            0x1B /* 108 Get/set USB configuration */

	/* UBX Class ESF External Sensor Fusion Messages */
#define UBX_CLASS_ESF_ALG            0x14 /* 16 Periodic/Polled IMU alignment information */
#define UBX_CLASS_ESF_INS            0x15 /* 36 Periodic/Polled Vehicle dynamics information */
#define UBX_CLASS_ESF_MEAS           0x02 /* (8 + 4*numM... Input/Output External sensor fusion measurements */
#define UBX_CLASS_ESF_RAW            0x03 /* 4 + 8*N Output Raw sensor measurements */
#define UBX_CLASS_ESF_STATUS         0x10 /* 16 + 4*numS... Periodic/Polled External sensor fusion status */

	/* UBX Class HNR High Rate Navigation Results Messages */
#define UBX_CLASS_HNR_ATT            0x01 /* 32 Periodic/Polled Attitude solution */
#define UBX_CLASS_HNR_INS            0x02 /* 36 Periodic/Polled Vehicle dynamics information */
#define UBX_CLASS_HNR_PVT            0x00 /* 72 Periodic/Polled High rate output of PVT solution */

	/* UBX Class INF Information Messages */
#define UBX_CLASS_INF_DEBUG          0x04 /* 0 + 1*N Output ASCII output with debug contents */
#define UBX_CLASS_INF_ERROR          0x00 /* 0 + 1*N Output ASCII output with error contents */
#define UBX_CLASS_INF_NOTICE         0x02 /* 0 + 1*N Output ASCII output with informational contents */
#define UBX_CLASS_INF_TEST           0x03 /* 0 + 1*N Output ASCII output with test contents */
#define UBX_CLASS_INF_WARNING        0x01 /* 0 + 1*N Output ASCII output with warning contents */

	/* UBX Class LOG Logging Messages */
#define UBX_CLASS_LOG_BATCH          0x11 /* 100 Polled Batched data */
#define UBX_CLASS_LOG_CREATE         0x07 /* 8 Command Create log file */
#define UBX_CLASS_LOG_ERASE          0x03 /* 0 Command Erase logged data */
	/* 12 Input Find index of a log entry based on a...
	 * 8 Output Response to FINDTIME request
	 */
#define UBX_CLASS_LOG_FINDTIME       0x0E
	/* 0 Poll Request Poll for log information
	 * 48 Output Log information
	 */
#define UBX_CLASS_LOG_INFO           0x08
#define UBX_CLASS_LOG_RETRIEVEBA     0x10 /* 4 Command Request batch data */
#define UBX_CLASS_LOG_RETRIEVEPO     0x0f /* 32 Output Odometer log entry */
#define UBX_CLASS_LOG_RETRIEVEPOS    0x0b /* 40 Output Position fix log entry */
#define UBX_CLASS_LOG_RETRIEVEST     0x0d /* 16 + 1*byteCo... Output Byte string log entry */
#define UBX_CLASS_LOG_RETRIEVE       0x09 /* 12 Command Request log data */
#define UBX_CLASS_LOG_STRING         0x04 /* 0 + 1*N Command Store arbitrary string in on-board flash */

	/* UBX Class MGA Multiple GNSS Assistance Messages */
#define UBX_CLASS_MGA_ACK_DATA0      0x60 /* 8 Output Multiple GNSS acknowledge message */
#define UBX_CLASS_MGA_ANO            0x20 /* 76 Input Multiple GNSS AssistNow Offline... */
#define UBX_CLASS_MGA_BDS_EPH        0x03 /* 88 Input BeiDou ephemeris assistance */
#define UBX_CLASS_MGA_BDS_ALM        0x03 /* 40 Input BeiDou almanac assistance */
#define UBX_CLASS_MGA_BDS_HEALTH     0x03 /* 68 Input BeiDou health assistance */
#define UBX_CLASS_MGA_BDS_UTC        0x03 /* 20 Input BeiDou UTC assistance */
#define UBX_CLASS_MGA_BDS_IONO       0x03 /* 16 Input BeiDou ionosphere assistance */
	/* 0 Poll Request Poll the navigation database
	 * 12 + 1*N Input/Output Navigation database dump entry
	 */
#define UBX_CLASS_MGA_DBD            0x80
#define UBX_CLASS_MGA_FLASH_DATA     0x21 /* 6 + 1*size Input Transfer MGA-ANO data block to flash */
#define UBX_CLASS_MGA_FLASH_STOP     0x21 /* 2 Input Finish flashing MGA-ANO data */
#define UBX_CLASS_MGA_FLASH_ACK      0x21 /* 6 Output Acknowledge last FLASH-DATA or -STOP */
#define UBX_CLASS_MGA_GAL_EPH        0x02 /* 76 Input Galileo ephemeris assistance */
#define UBX_CLASS_MGA_GAL_ALM        0x02 /* 32 Input Galileo almanac assistance */
#define UBX_CLASS_MGA_GAL_TIMEO      0x02 /* 12 Input Galileo GPS time offset assistance */
#define UBX_CLASS_MGA_GAL_UTC        0x02 /* 20 Input Galileo UTC assistance */
#define UBX_CLASS_MGA_GLO_EPH        0x06 /* 48 Input GLONASS ephemeris assistance */
#define UBX_CLASS_MGA_GLO_ALM        0x06 /* 36 Input GLONASS almanac assistance */
#define UBX_CLASS_MGA_GLO_TIMEO      0x06 /* 20 Input GLONASS auxiliary time offset assistance */
#define UBX_CLASS_MGA_GPS_EPH        0x00 /* 68 Input GPS ephemeris assistance */
#define UBX_CLASS_MGA_GPS_ALM        0x00 /* 36 Input GPS almanac assistance */
#define UBX_CLASS_MGA_GPS_HEALTH     0x00 /* 40 Input GPS health assistance */
#define UBX_CLASS_MGA_GPS_UTC        0x00 /* 20 Input GPS UTC assistance */
#define UBX_CLASS_MGA_GPS_IONO       0x00 /* 16 Input GPS ionosphere assistance */
#define UBX_CLASS_MGA_INI_POS_XYZ    0x40 /* 20 Input Initial position assistance */
#define UBX_CLASS_MGA_INI_POS_LLH    0x40 /* 20 Input Initial position assistance */
#define UBX_CLASS_MGA_INI_TIME_UTC   0x40 /* 24 Input Initial time assistance */
#define UBX_CLASS_MGA_INI_TIME_GN    0x40 /* 24 Input Initial time assistance */
#define UBX_CLASS_MGA_INI_CLKD       0x40 /* 12 Input Initial clock drift assistance */
#define UBX_CLASS_MGA_INI_FREQ       0x40 /* 12 Input Initial frequency assistance */
#define UBX_CLASS_MGA_INI_EOP        0x40 /* 72 Input Earth orientation parameters assistance */
#define UBX_CLASS_MGA_QZSS_EPH       0x05 /* 68 Input QZSS ephemeris assistance */
#define UBX_CLASS_MGA_QZSS_ALM       0x05 /* 36 Input QZSS almanac assistance */
#define UBX_CLASS_MGA_QZSS_HEAL      0x05 /* 12 Input QZSS health assistance */

	/* UBX Class MON Monitoring Messages */
#define UBX_CLASS_MON_BATCH          0x32 /* 12 Polled Data batching buffer status */
#define UBX_CLASS_MON_GNSS           0x28 /* 8 Polled Information message major GNSS... */
#define UBX_CLASS_MON_HW2            0x0B /* 28 Periodic/Polled Extended hardware status */
#define UBX_CLASS_MON_HW             0x09 /* 60 Periodic/polled Hardware status */
#define UBX_CLASS_MON_IO             0x02 /* 0 + 20*N Periodic/Polled I/O system status */
#define UBX_CLASS_MON_MSGPP          0x06 /* 120 Periodic/Polled Message parse and process status */
	/* 0 Poll Request Poll request for installed patches
	 * 4 + 16*nEntriesPolled Installed patches
	 */
#define UBX_CLASS_MON_PATCH          0x27
#define UBX_CLASS_MON_RXBUF          0x07 /* 24 Periodic/Polled Receiver buffer status */
#define UBX_CLASS_MON_RXR            0x21 /* 1 Output Receiver status information */
#define UBX_CLASS_MON_SMGR           0x2E /* 16 Periodic/Polled Synchronization manager status */
#define UBX_CLASS_MON_SPT            0x2F /* 4 + 12*numR... Polled Sensor production test */
#define UBX_CLASS_MON_TXBUF          0x08 /* 28 Periodic/Polled Transmitter buffer status */
	/* 0 Poll Request Poll receiver and software version
	 * 40 + 30*N Polled Receiver and software version
	 */
#define UBX_CLASS_MON_VER            0x04

	/* UBX Class NAV Navigation Results Messages */
#define UBX_CLASS_NAV_AOPSTATUS      0x60 /* 16 Periodic/Polled AssistNow Autonomous status */
#define UBX_CLASS_NAV_ATT            0x05 /* 32 Periodic/Polled Attitude solution */
#define UBX_CLASS_NAV_CLOCK          0x22 /* 20 Periodic/Polled Clock solution */
#define UBX_CLASS_NAV_COV            0x36 /* 64 Periodic/Polled Covariance matrices */
#define UBX_CLASS_NAV_DGPS           0x31 /* 16 + 12*numCh Periodic/Polled DGPS data used for NAV */
#define UBX_CLASS_NAV_DOP            0x04 /* 18 Periodic/Polled Dilution of precision */
#define UBX_CLASS_NAV_EELL           0x3d /* 16 Periodic/Polled Position error ellipse parameters */
#define UBX_CLASS_NAV_EOE            0x61 /* 4 Periodic End of epoch */
#define UBX_CLASS_NAV_GEOFENCE       0x39 /* 8 + 2*numFe... Periodic/Polled Geofencing status */
#define UBX_CLASS_NAV_HPPOSECEF      0x13 /* 28 Periodic/Polled High precision position solution in ECEF */
#define UBX_CLASS_NAV_HPPOSLLH       0x14 /* 36 Periodic/Polled High precision geodetic position solution */
#define UBX_CLASS_NAV_NMI            0x28 /* 16 Periodic/Polled Navigation message cross-check... */
#define UBX_CLASS_NAV_ODO            0x09 /* 20 Periodic/Polled Odometer solution */
#define UBX_CLASS_NAV_ORB            0x34 /* 8 + 6*numSv Periodic/Polled GNSS orbit database info */
#define UBX_CLASS_NAV_POSECEF        0x01 /* 20 Periodic/Polled Position solution in ECEF */
#define UBX_CLASS_NAV_POSLLH         0x02 /* 28 Periodic/Polled Geodetic position solution */
#define UBX_CLASS_NAV_PVT            0x07 /* 92 Periodic/Polled Navigation position velocity time solution */
#define UBX_CLASS_NAV_RELPOSNED      0x3C /* 40 Periodic/Polled Relative positioning information in... */
#define UBX_CLASS_NAV_RESETODO       0x10 /* 0 Command Reset odometer */
#define UBX_CLASS_NAV_SAT            0x35 /* 8 + 12*numSvs Periodic/Polled Satellite information */
#define UBX_CLASS_NAV_SBAS           0x32 /* 12 + 12*cnt Periodic/Polled SBAS status data */
#define UBX_CLASS_NAV_SLAS           0x42 /* 20 + 8*cnt Periodic/Polled QZSS L1S SLAS status data */
#define UBX_CLASS_NAV_SOL            0x06 /* 52 Periodic/Polled Navigation solution information */
#define UBX_CLASS_NAV_STATUS         0x03 /* 16 Periodic/Polled Receiver navigation status */
#define UBX_CLASS_NAV_SVINFO         0x30 /* 8 + 12*numCh Periodic/Polled Space vehicle information */
#define UBX_CLASS_NAV_SVIN           0x3B /* 40 Periodic/Polled Survey-in data */
#define UBX_CLASS_NAV_TIMEBDS        0x24 /* 20 Periodic/Polled BeiDou time solution */
#define UBX_CLASS_NAV_TIMEGAL        0x25 /* 20 Periodic/Polled Galileo time solution */
#define UBX_CLASS_NAV_TIMEGLO        0x23 /* 20 Periodic/Polled GLONASS time solution */
#define UBX_CLASS_NAV_TIMEGPS        0x20 /* 16 Periodic/Polled GPS time solution */
#define UBX_CLASS_NAV_TIMELS         0x26 /* 24 Periodic/Polled Leap second event information */
#define UBX_CLASS_NAV_TIMEUTC        0x21 /* 20 Periodic/Polled UTC time solution */
#define UBX_CLASS_NAV_VELECEF        0x11 /* 20 Periodic/Polled Velocity solution in ECEF */
#define UBX_CLASS_NAV_VELNED         0x12 /* 36 Periodic/Polled Velocity solution in NED frame */

	/* UBX Class RXM Receiver Manager Messages */
#define UBX_CLASS_RXM_IMES           0x61 /* 4 + 44*numTx Periodic/Polled Indoor Messaging System information */
#define UBX_CLASS_RXM_MEASX          0x14 /* 44 + 24*num... Periodic/Polled Satellite measurements for RRLP */
	/* 8 Command Power management request
	 * 16 Command Power management request
	 */
#define UBX_CLASS_RXM_PMREQ          0x41
	/* 16 + 32*num... Periodic/Polled Multi-GNSS raw measurement data
	 * 16 + 32*num... Periodic/Polled Multi-GNSS raw measurements
	 */
#define UBX_CLASS_RXM_RAWX           0x15
	/* 16 Output Galileo SAR short-RLM report
	 * 28 Output Galileo SAR long-RLM report
	 */
#define UBX_CLASS_RXM_RLM            0x59
#define UBX_CLASS_RXM_RTCM           0x32 /* 8 Output RTCM input status */
	/* 8 + 4*numW... Output Broadcast navigation data subframe
	 * 8 + 4*numW... Output Broadcast navigation data subframe
	 */
#define UBX_CLASS_RXM_SFRBX          0x13
#define UBX_CLASS_RXM_SVSI           0x20 /* 8 + 6*numSV Periodic/Polled SV status info */

	/* UBX Class SEC Security Feature Messages */
#define UBX_CLASS_SEC_UNIQID         0x03 /* 9 Output Unique chip ID */

	/* UBX Class TIM Timing Messages */
#define UBX_CLASS_TIM_DOSC           0x11 /* 8 Output Disciplined oscillator control */
#define UBX_CLASS_TIM_FCHG           0x16 /* 32 Periodic/Polled Oscillator frequency changed notification */
#define UBX_CLASS_TIM_HOC            0x17 /* 8 Input Host oscillator control */
#define UBX_CLASS_TIM_SMEAS          0x13 /* 12 + 24*num... Input/Output Source measurement */
#define UBX_CLASS_TIM_SVIN           0x04 /* 28 Periodic/Polled Survey-in data */
#define UBX_CLASS_TIM_TM2            0x03 /* 28 Periodic/Polled Time mark data */
#define UBX_CLASS_TIM_TOS            0x12 /* 56 Periodic Time pulse time and frequency data */
#define UBX_CLASS_TIM_TP             0x01 /* 16 Periodic/Polled Time pulse time data */
	/* 1 Command Stop calibration
	 * 12 Command VCO calibration extended command
	 * 12 Periodic/Polled Results of the calibration
	 */
#define UBX_CLASS_TIM_VCOCAL         0x15
#define UBX_CLASS_TIM_VRFY           0x06 /* 20 Periodic/Polled Sourced time verification */
	/* UBX Class UPD Firmware Update Messages */
	/* 0 Poll Request Poll backup restore status
	 * 4 Command Create backup in flash
	 * 4 Command Clear backup in flash
	 * 8 Output Backup creation acknowledge
	 * 8 Output System restored from backup
	 */
#define UBX_CLASS_UPD_SOS            0x14

	u16 len;
	u8 *buffer;

	u8 check_sum_a;
	u8 check_sum_b;
};

static int process_nav(u8 id, void *buffer, u32 len)
{
	int ret;

	switch (id) {
	case UBX_CLASS_NAV_AOPSTATUS:
		break;
	case UBX_CLASS_NAV_ATT:
		break;
	case UBX_CLASS_NAV_CLOCK:
		break;
	case UBX_CLASS_NAV_COV:
		break;
	case UBX_CLASS_NAV_DGPS:
		break;
	case UBX_CLASS_NAV_DOP:
		break;
	case UBX_CLASS_NAV_EELL:
		break;
	case UBX_CLASS_NAV_EOE:
		break;
	case UBX_CLASS_NAV_GEOFENCE:
		break;
	case UBX_CLASS_NAV_HPPOSECEF:
		break;
	case UBX_CLASS_NAV_HPPOSLLH:
		break;
	case UBX_CLASS_NAV_NMI:
		break;
	case UBX_CLASS_NAV_ODO:
		break;
	case UBX_CLASS_NAV_ORB:
		break;
	case UBX_CLASS_NAV_POSECEF:
		break;
	case UBX_CLASS_NAV_POSLLH:
		break;
	case UBX_CLASS_NAV_PVT:
		break;
	case UBX_CLASS_NAV_RELPOSNED:
		break;
	case UBX_CLASS_NAV_RESETODO:
		break;
	case UBX_CLASS_NAV_SAT:
		break;
	case UBX_CLASS_NAV_SBAS:
		break;
	case UBX_CLASS_NAV_SLAS:
		break;
	case UBX_CLASS_NAV_SOL:
		break;
	case UBX_CLASS_NAV_STATUS:
		break;
	case UBX_CLASS_NAV_SVINFO:
		break;
	case UBX_CLASS_NAV_SVIN:
		break;
	case UBX_CLASS_NAV_TIMEBDS:
		break;
	case UBX_CLASS_NAV_TIMEGAL:
		break;
	case UBX_CLASS_NAV_TIMEGLO:
		break;
	case UBX_CLASS_NAV_TIMEGPS:
		break;
	case UBX_CLASS_NAV_TIMELS:
		break;
	case UBX_CLASS_NAV_TIMEUTC:
		break;
	case UBX_CLASS_NAV_VELECEF:
		break;
	case UBX_CLASS_NAV_VELNED:
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

int ubx_decode_link_partner(void *buffer, u32 len)
{
	u8 *check_buff = buffer;
	u8 check_sum = 0;
	u8 check_add = 0;
	u8 i;
	int ret;

	for (i = 0; i < len - 2; i++) {
		check_sum += check_buff[i];
		check_add += check_sum;
	}

	if (check_sum != check_buff[len - 2] || check_add != check_buff[len - 1]) {
		return -EPIPE;
	}

	if (UBX_HEAD != *(u16 *)check_buff) {
		return -EPIPE;
	}

	switch (check_buff[2]) {
	case UBX_CLASS_NAV:
		ret = process_nav(check_buff[3], &check_buff[6], *(((u16 *)check_buff) + 2));
		break;
	case UBX_CLASS_RXM:
		break;
	case UBX_CLASS_INF:
		break;
	case UBX_CLASS_ACK:
		break;
	case UBX_CLASS_CFG:
		break;
	case UBX_CLASS_UPD:
		break;
	case UBX_CLASS_MON:
		break;
	case UBX_CLASS_AID:
		break;
	case UBX_CLASS_TIM:
		break;
	case UBX_CLASS_ESF:
		break;
	case UBX_CLASS_MGA:
		break;
	case UBX_CLASS_LOG:
		break;
	case UBX_CLASS_SEC:
		break;
	case UBX_CLASS_HNR:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

void ubx_receive_frame(u8 data, void *buffer, u8 *done)
{
	u8 *buf = buffer;
	static u8 cnt;
	static ktime_t time;
	u16 i, len = 0;
	int ret;

	if (ktime_us_delta(ktime_get(), time) > 2500 && time) {
		cnt = 0;
	}
	time = ktime_get();

	buf[cnt++] = data;

	if (buf[0] != 0xB5 || buf[1] == 0x62) {
		cnt = 0;
	}
	if (buf[2] < UBX_CLASS_NAV || buf[2] > UBX_CLASS_HNR) {
		cnt = 0;
	}

	if (cnt == 6) {
		len = *(((u16 *)buf) + 2);
	} else if (cnt == len + 6 + 2) {
		*done = 1;
	} else {
		*done = 0;
	}
}
