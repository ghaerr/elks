#ifndef TCP_CB_H
#define TCP_CB_H

extern int tcpcb_num;

void tcpcb_init(void);
struct tcpcb_list_s *tcpcb_clone(struct tcpcb_s *cb);
void tcpcb_remove(struct tcpcb_list_s *n);
void tcpcb_remove_cb(struct tcpcb_s *cb);
void tcpcb_buf_read(struct tcpcb_s *cb, unsigned char *data, int len);
void tcpcb_buf_write(struct tcpcb_s *cb, unsigned char *data, int len);
void tcpcb_expire_timeouts(void);
void tcpcb_push_data(void);
struct tcpcb_list_s *tcpcb_check_port(__u16 lport);
struct tcpcb_list_s *tcpcb_find_unaccepted(void *sock);
void tcpcb_rmv_all_unaccepted(struct tcpcb_s *cb);
struct tcpcb_s *tcpcb_getbynum(int num);
void tcpcb_printall(void);

#endif
