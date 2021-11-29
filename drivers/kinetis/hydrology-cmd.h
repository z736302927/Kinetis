#ifndef  _HYDROLOGYCOMMAND_H_
#define  _HYDROLOGYCOMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

int hydrology_execute_command(enum hydrology_body_type funcode);
int hydrology_response_downstream(enum hydrology_body_type funcode);
int hydrology_response_upstream(enum hydrology_body_type funcode, u8 End);
int hydrology_device_reset(void);
int hydrology_host_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* _HYDROLOGYCOMMAND_H_ */
