#define ALT111_TARGET_MHI2Q 1
#include "../altscreen_hook/libaltscreen111_gen2.c"
#include <assert.h>
#include <sys/resource.h>

static void test_high_fd(void){
    int fds[2],high;char received[4];struct rlimit lim;
    assert(!getrlimit(RLIMIT_NOFILE,&lim));
    if(lim.rlim_cur<512){lim.rlim_cur=512;assert(!setrlimit(RLIMIT_NOFILE,&lim));}
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,fds));high=fcntl(fds[0],F_DUPFD,300);assert(high>=300);close(fds[0]);
    g_dongle_alt_cid=0;assert(write(fds[1],"test",4)==4);assert(read_exact(high,received,4)==1);
    assert(!memcmp(received,"test",4));assert(!send_view_msg(high,(const uint8_t *)"view",4));
    assert(read(fds[1],received,4)==4 && !memcmp(received,"view",4));close(high);close(fds[1]);
}
static void test_timeout(void){
    int fds[2];uint8_t bytes[4096]={0};ssize_t n;uint64_t start;
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,fds));
    assert(!fcntl(fds[0],F_SETFL,O_NONBLOCK));
    do{n=send(fds[0],bytes,sizeof bytes,MSG_DONTWAIT);}while(n>0);
    assert(errno==EAGAIN || errno==EWOULDBLOCK);errno=0;start=monotonic_ms();
    assert(send_view_msg(fds[0],bytes,sizeof bytes)==-1 && errno==ETIMEDOUT);
    assert(monotonic_ms()-start>=190 && monotonic_ms()-start<1500);close(fds[0]);close(fds[1]);
}
static void test_legacy_controls(void){
    int fds[2];uint8_t msg[16];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,fds));
    g_dongle_alt_cid=123;g_alt_bc_last_view=-1;g_desired_view=2;g_alt_bc_keyframe=1;
    assert(!dongle_send_controls(fds[0]));assert(read(fds[1],msg,sizeof msg)==16);
    assert(!memcmp(msg,"VIEW\2\0\0\0KEYF\0\0\0\0",16));
    assert(sender_listed("iPhone9,1","18D70") && !sender_listed("HW301","18D70"));
    close(fds[0]);close(fds[1]);g_dongle_alt_cid=0;
}
int main(void){alarm(10);test_high_fd();test_timeout();test_legacy_controls();alarm(0);puts("OK: high-fd native transport, bounded send and legacy compatibility");return 0;}
