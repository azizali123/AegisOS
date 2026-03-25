#ifndef SESSION_H
#define SESSION_H

/* Oturum yonetimi */
void session_init(void);
int  session_login_screen(void);  /* 1: basarili, 0: iptal */
void session_logout(void);
void session_request_logout(void);
int  session_is_logout_requested(void);

#endif /* SESSION_H */
