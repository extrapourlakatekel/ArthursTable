#ifndef PAINTERFACE_H
#define PAINTERFACE_H
#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

bool pa_init(char * id, char * od, uint32_t framesize, uint32_t samplerate, int latencylevel);
void pa_exit(void);
void pa_readframe(float *in);
void pa_writeframe(float *out);



#ifdef __cplusplus
}
#endif


#endif
