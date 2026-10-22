#ifndef FEAR_SERVER_HOST_H
#define FEAR_SERVER_HOST_H

class ILTMessage_Read;

#ifdef __cplusplus
extern "C" {
#endif

bool FearServerHost_Start(const char *pszWorldName);
void FearServerHost_Update(void);
void FearServerHost_DeliverMessages(void);
void FearServerHost_Shutdown(void);
bool FearServerHost_IsRunning(void);

typedef void (*TFearServerMessageSink)(ILTMessage_Read *pMessage);
void FearServerHost_SetMessageSink(TFearServerMessageSink pSink);

#ifdef __cplusplus
}
#endif

#endif
