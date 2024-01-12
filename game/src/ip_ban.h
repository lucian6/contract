#ifndef __INC_BAN_IP_H__
#define __INC_BAN_IP_H__

extern bool LoadBanIP(const char* filename);
extern bool IsBanIP(struct in_addr in);
#if defined(__IMPROVED_HANDSHAKE_PROCESS__)
extern bool BanIP(struct in_addr in, const char* c_szIP);
extern bool AddBanIP(const char* c_szIP);
#endif

#endif // __INC_BAN_IP_H__