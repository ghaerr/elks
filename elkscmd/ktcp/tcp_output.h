#ifndef TCP_OUTPUT_H
#define TCP_OUTPUT_H

void tcp_retrans_expire(void);
void tcp_retrans_retransmit(void);
void rmv_all_retrans(struct tcpcb_list_s *lcb);
void rmv_all_retrans_cb(struct tcpcb_s *cb);

#endif
