#ifndef RC_API_H
#define RC_API_H
#ifdef __cplusplus
extern "C" {
#endif
void rc_init(void);
void rc_poll(void);
void rc_safety_tick(void);
void rc_emergency_cutoff(void);
#ifdef __cplusplus
}
#endif
#endif
