#ifndef TCP_OUTPUT_H
#define TCP_OUTPUT_H

void tcp_retrans(void);
void rmv_all_retrans(struct tcpcb_list_s *lcb);

#endif
