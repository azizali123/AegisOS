#include "../include/string.h"
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
char*  strcpy(char *d,const char *s){char *r=d;while((*d++=*s++));return r;}
char*  strncpy(char *d,const char *s,size_t n){
    size_t i=0;for(;i<n&&s[i];i++)d[i]=s[i];for(;i<n;i++)d[i]=0;return d;}
int    strcmp(const char *a,const char *b){
    while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
int    strncmp(const char *a,const char *b,size_t n){
    for(size_t i=0;i<n;i++){if(!a[i]&&!b[i])return 0;if(a[i]!=b[i])return (unsigned char)a[i]-(unsigned char)b[i];}return 0;}
char*  strcat(char *d,const char *s){char *r=d;while(*d)d++;while((*d++=*s++));return r;}
char*  strchr(const char *s,int c){for(;*s;s++)if(*s==(char)c)return(char*)s;return 0;}
void*  memset(void *d,int c,size_t n){uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)c;return d;}
void*  memcpy(void *d,const void *s,size_t n){
    uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++;return d;}
int    memcmp(const void *a,const void *b,size_t n){
    const uint8_t *p=(const uint8_t*)a,*q=(const uint8_t*)b;
    for(size_t i=0;i<n;i++)if(p[i]!=q[i])return p[i]-q[i];return 0;}

void int_to_str(int v,char *buf){
    if(v<0){*buf++='-';v=-v;}
    char tmp[12];int i=0;
    if(v==0){buf[0]='0';buf[1]=0;return;}
    while(v>0){tmp[i++]=(char)('0'+v%10);v/=10;}
    for(int j=0;j<i;j++){buf[j]=tmp[i-1-j];}buf[i]=0;
}
void uint_to_hex(uint32_t v,char *buf){
    const char *h="0123456789ABCDEF";
    buf[0]='0';buf[1]='x';
    for(int i=9;i>=2;i--){buf[i]=h[v&0xF];v>>=4;}
    buf[10]=0;
}
