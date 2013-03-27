#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <algorithm>

extern "C" {
#include <coroutine/yfr_coroutine.h>
#include <syscall_hook/yfr_syscall.h>
#include <syscall_hook/yfr_dlsym.h>
#include <syscall_hook/yfr_ipc.h>
#include <log_ext/yf_log_file.h>
}


yf_pool_t *_mem_pool;
yf_log_t* _log;
yf_evt_driver_t *_evt_driver = NULL;
yfr_coroutine_mgr_t* _test_coroutine_mgr = NULL;

yf_s32_t  _test_switch_total = 0;

void _test_on_poll_evt_driver(yf_evt_driver_t* evt_driver, void* data, yf_log_t* log)
{
        if (_test_switch_total == 0)
        {
                yf_evt_driver_stop(evt_driver);
                return;
        }
        yfr_coroutine_schedule(_test_coroutine_mgr);
}

yf_time_t  _test_stack_watch_tm;

void _test_coroutine_stack_watch_handle(struct yf_tm_evt_s* evt, yf_time_t* start)
{
        yf_u32_t  min_percent = yfr_coroutine_stack_check(_log);
        
        printf("coroutine stack left percent=%d/%d\n", 
                        min_percent, YFR_COROUTINE_STACK_WATCH_SPLIT);
        
        yf_register_tm_evt(evt, &_test_stack_watch_tm);
}

class CoroutineTestor : public testing::Test
{
public:
        virtual ~CoroutineTestor()
        {
        }
        virtual void SetUp()
        {
                yf_evt_driver_init_t driver_init = {0, 
                                128, 1024, _log, YF_DEFAULT_DRIVER_CB};
                
                driver_init.poll_cb = _test_on_poll_evt_driver;
                
                _evt_driver = yf_evt_driver_create(&driver_init);
                ASSERT_TRUE(_evt_driver != NULL);
                yf_log_file_flush_drive(_evt_driver, 5000, NULL);

                //cause epoll wil ret error if no fd evts...
                yf_fd_t fds[2];
                socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
                yf_fd_event_t* fd_evts[2];
                yf_alloc_fd_evt(_evt_driver, fds[0], fd_evts, fd_evts + 1, _log);
                yf_register_fd_evt(fd_evts[0], NULL);

                //add coroutine stack watch timer
                yf_tm_evt_t* tm_evt = NULL;
                yf_alloc_tm_evt(_evt_driver, &tm_evt, _log);
                tm_evt->timeout_handler = _test_coroutine_stack_watch_handle;
                yf_ms_2_time(3000, &_test_stack_watch_tm);
                yf_register_tm_evt(tm_evt, &_test_stack_watch_tm);

                //init global coroutine set
                yf_int_t ret = yfr_coroutine_global_set(128, 4096*8, 0, _log);
                assert(ret == 0);
                
                //init coroutine _test_coroutine_mgr
                yfr_coroutine_init_t init_info = {128, 4, 64, _evt_driver};
                _test_coroutine_mgr = yfr_coroutine_mgr_create(&init_info, _log);
                assert(_test_coroutine_mgr);

                yfr_syscall_coroutine_attach(_test_coroutine_mgr, _log);

                //should after mgr created
                yf_log_file_add_handle(_log, 'r', yfr_coroutine_log);
        }

        virtual void TearDown()
        {
                yf_evt_driver_destory(_evt_driver);
        }        
};

/*
* 101 million switch
* if in undebug mode, test result is:
* on my home computer's virtual linux sys:
* [       OK ] CoroutineTestor.Switch (9173|8970|9121|9033|9374 ms)
* on a computer with 8 cpus with real 64bit linux sys:
* [       OK ] CoroutineTestor.Switch (6571|6643|6859|6620|6629|6771 ms)
*
* if on debug with -O0 compile, too slow
[       OK ] CoroutineTestor.Switch (13118 ms)
*/
yf_int_t _coroutine_switch_proc(yfr_coroutine_t* r)
{
        int icnt = *(int*)r->arg;
        printf("r id=%lld, switch cnt=%d\n", r->id, icnt);
        while (icnt)
        {
                --icnt;
                --_test_switch_total;
                yfr_coroutine_yield(r);
        }
        printf("r id=%lld,  exit\n", r->id);
        return  0;
}

/*
* if no evt dirver and core in main thread, log cant flush
*/
static void _test_reset_switch(int* icnt, int size, int area)
{
        _test_switch_total = 0;
        for (int i = 0; i < size; ++i)
        {
                icnt[i] = (i + 1) * area;
                _test_switch_total += icnt[i];
        }
        std::random_shuffle(icnt, icnt + (YF_ARRAY_SIZE(icnt) - 1));        
}


TEST_F(CoroutineTestor, Switch)
{
        int icnt[100];
        _test_reset_switch(icnt, YF_ARRAY_SIZE(icnt), 20000);

        printf("switch total cnt=%d\n", _test_switch_total);

        yfr_coroutine_t* r = NULL;
        for (int i = 0; i < YF_ARRAY_SIZE(icnt); ++i)
        {
                r = yfr_coroutine_create(_test_coroutine_mgr, _coroutine_switch_proc, icnt+i, _log);
                assert(r);
        }
        
        while (_test_switch_total > 0)
        {
                yfr_coroutine_schedule(_test_coroutine_mgr);
        }
}

yf_int_t _coroutine_sleep_proc(yfr_coroutine_t* r)
{
        int icnt = *(int*)r->arg;
        printf("r id=%lld, sleep cnt=%d\n", r->id, icnt);
        while (icnt)
        {
                --icnt;
                --_test_switch_total;
                if ((icnt & 15) == 0)
                        usleep(yf_mod(random(), 1024));
        }
        printf("r id=%lld,  exit\n", r->id);
        return 0;
}

TEST_F(CoroutineTestor, Sleep)
{       
        int icnt[100];
        _test_reset_switch(icnt, YF_ARRAY_SIZE(icnt), 10);

        printf("sleep total cnt=%d\n", _test_switch_total);

        yfr_coroutine_t* r = NULL;
        for (int i = 0; i < YF_ARRAY_SIZE(icnt); ++i)
        {
                r = yfr_coroutine_create(_test_coroutine_mgr, _coroutine_sleep_proc, icnt+i, _log);
                assert(r);
        }
        
        yf_evt_driver_start(_evt_driver);
}


yfr_ipc_lock_t  _test_lock;

yf_int_t _coroutine_lock_proc(yfr_coroutine_t* r)
{
        int icnt = *(int*)r->arg;
        usleep(yf_mod(random(), 4096));

        int lock_cnt = yf_mod(random(), 7);
        lock_cnt = yf_max(1, lock_cnt);

        printf("r id=%lld, lock cnt in lock=%d\n", r->id, lock_cnt);
        
        yf_int_t ret = 0;
        int i = 0;
        for (; i < lock_cnt / 2; ++i)
        {
                yfr_ipc_lock(&_test_lock, 0, NULL);
                assert(ret == YF_OK);
        }
        
        while (icnt)
        {
                --icnt;
                --_test_switch_total;
                if ((icnt & 31) == 0)
                {
                        printf("r id=%lld, sleep now\n", r->id);
                        usleep(yf_mod(random(), 1024));
                }
                if (i < lock_cnt)
                {
                        yfr_ipc_lock(&_test_lock, 0, NULL);
                        assert(ret == YF_OK);
                        ++i;
                }
        }

        for (--i; i >= 0; --i)
                yfr_ipc_unlock(&_test_lock);
        printf("r id=%lld,  exit\n", r->id);
        return  0;
}


TEST_F(CoroutineTestor, IpcLock)
{
        yfr_ipc_lock_init(&_test_lock);
        
        int icnt[32];
        _test_reset_switch(icnt, YF_ARRAY_SIZE(icnt), 1);

        printf("sleep total cnt=%d\n", _test_switch_total);

        yfr_coroutine_t* r = NULL;
        for (int i = 0; i < YF_ARRAY_SIZE(icnt); ++i)
        {
                r = yfr_coroutine_create(_test_coroutine_mgr, _coroutine_lock_proc, icnt+i, _log);
                assert(r);
        }
        
        yf_evt_driver_start(_evt_driver);        
}


yfr_ipc_cond_t  _test_cond;
std::list<yf_u32_t>  _notify_queue;

yf_int_t _coroutine_cond_produce_proc(yfr_coroutine_t* r)
{
        int icnt = *(int*)r->arg;
        printf("r id=%lld, produce cnt=%d\n", r->id, icnt);
        while (icnt)
        {
                --icnt;
                _notify_queue.push_back(yf_mod(random(), 4096));
                
                if (yfr_ipc_cond_have_waits(&_test_cond))
                {
                        printf("consume wait, sig it from sleep now\n");
                        yfr_ipc_cond_sig(&_test_cond);
                }

                if ((icnt & 3) == 0)
                        usleep(yf_mod(random(), 8192*2));
        }
        printf("r id=%lld,  exit\n", r->id);
        return 0;
}


yf_int_t _coroutine_cond_consume_proc(yfr_coroutine_t* r)
{
        while (_test_switch_total)
        {
                int icnt = 0;
                while (!_notify_queue.empty())
                {
                        _notify_queue.pop_front();
                        --_test_switch_total;
                        if ((++icnt % 64) == 0)
                        {
                                printf("consume sleep now, rest cnt=%d...\n", _test_switch_total);
                                usleep(yf_mod(random(), 1024));
                        }
                }
                
                printf("queue empty, consume should wait now...\n");
                
                yfr_ipc_cond_wait(&_test_cond);
        }
        return 0;
}


TEST_F(CoroutineTestor, IpcCond)
{
        yfr_ipc_cond_init(&_test_cond);
        
        int icnt[25];
        _test_reset_switch(icnt, YF_ARRAY_SIZE(icnt), 10);

        printf("cond total cnt=%d\n", _test_switch_total);

        yfr_coroutine_t* r = NULL, * cmpr = NULL;
        for (int i = 0; i < YF_ARRAY_SIZE(icnt); ++i)
        {
                r = yfr_coroutine_create(_test_coroutine_mgr, 
                                _coroutine_cond_produce_proc, icnt+i, _log);

                assert(r);
                cmpr = yfr_coroutine_getby_id(_test_coroutine_mgr, r->id);
                ASSERT_EQ(cmpr, r);
        }

        r = yfr_coroutine_create(_test_coroutine_mgr, 
                                _coroutine_cond_consume_proc,  NULL, _log);
        assert(r);
        
        yf_evt_driver_start(_evt_driver);
}



yf_int_t _coroutine_serach_kwd(yfr_coroutine_t* r)
{
        int isearch = *(int*)r->arg;
        printf("r id=%lld, serach=%d\n", r->id, isearch);
        yf_int_t  ret = 0, head_recved = 0;

        char  send_buf[2048] = {0};
        int send_len = sprintf(send_buf, "GET /s?wd=%d HTTP/1.0\r\nUser-Agent: Wget/1.11.4 Red Hat modified\r\n"
                                "Accept: */*\r\nHost: www.baidu.com\r\nConnection: Close\r\n\r\n", 
                                isearch);
        int  all_recv_len = 0, body_len = 82888899, body_recv = 0;
        
        yf_utime_t  utime;
        yf_ms_2_utime(1000 * 12, &utime);

        yf_sockaddr_storage_t sock_addr;
        yf_memzero_st(sock_addr);
        sock_addr.ss_family = AF_INET;
        yf_sock_set_addr((yf_sock_addr_t*)&sock_addr, "220.181.111.148");
        yf_sock_set_port((yf_sock_addr_t*)&sock_addr, 80);

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        assert(sockfd);
        yfr_socket_conn_tmset(sockfd, 1000 * 25);
        
        ret = connect(sockfd, (yf_sock_addr_t*)&sock_addr, 
                        yf_sock_len((yf_sock_addr_t*)&sock_addr));
        if (ret != 0)
        {
                printf("connect failed, err=%d, desc=%s]\n", yf_errno, strerror(yf_errno));
                goto end;
        }
        
        ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &utime, sizeof(utime));
        assert(ret == 0);

        //printf("req send len=%d\n", send_len);
        
        ret = write(sockfd, send_buf, send_len);
        if (ret < 0)
        {
                printf("write failed, err=%d]\n", yf_errno);
                goto end;
        }
        assert(ret == send_len);
        
        while ((ret = read(sockfd, send_buf, sizeof(send_buf) - 1)) > 0)
        {
                send_buf[ret] = 0;
                fprintf(stderr, "r id=%lld, recv=-----\n%s\n_______\n", r->id, send_buf);
                
                all_recv_len += ret;
                if (!head_recved)
                {
                        char* cl = strcasestr(send_buf, "Content-Length: ");
                        if (cl)
                                body_len = atoi(cl + strlen("Content-Length: "));
                        cl = strstr(send_buf, "\r\n\r\n");
                        if (cl)
                        {
                                head_recved = 1;
                                body_recv = send_buf + ret - (cl + 4);
                        }
                }
                else {
                        body_recv += ret;
                        if (body_recv >= body_len) 
                        {
                                printf("r id=%lld recv all body, len=%d\n", r->id, body_len);
                                break;
                        }
                }
        }
        
        if (ret <= 0) {
                printf("r id=%lld recv ret errrrrrrrrrrrrrrrrrrrr=%d, all_recv_len=%d, body_len=%d, "
                        "err=%d, str='%s'\n", r->id, 
                        ret, all_recv_len, body_len, 
                        yf_errno, strerror(yf_errno));
        }

end:
        close(sockfd);
        printf("r id=%lld,  exit\n", r->id);
        --_test_switch_total;
        return 0;
}


TEST_F(CoroutineTestor, SocketActive)
{
        yfr_coroutine_t* r = NULL;
        int iserach[36];
        
        _test_switch_total = YF_ARRAY_SIZE(iserach);
        for (int i = 0; i < YF_ARRAY_SIZE(iserach); ++i)
        {
                iserach[i] = yf_mod(random(), 8192*16);
                
                r = yfr_coroutine_create(_test_coroutine_mgr, 
                                _coroutine_serach_kwd, 
                                iserach+i, _log);
                assert(r);
        }
        yf_evt_driver_start(_evt_driver);
}


#ifdef TEST_F_INIT
TEST_F_INIT(CoroutineTestor, Switch);
TEST_F_INIT(CoroutineTestor, Sleep);
TEST_F_INIT(CoroutineTestor, IpcCond);
TEST_F_INIT(CoroutineTestor, IpcLock);
TEST_F_INIT(CoroutineTestor, SocketActive);
#endif

int main(int argc, char **argv)
{
        srandom(time(NULL));
        yf_pagesize = getpagesize();
        printf("pagesize=%d\n", yf_pagesize);

        yf_cpuinfo();

        yf_int_t ret = yfr_syscall_init();
        assert(ret == YF_OK);

        //first need init threads before init log file
        ret = yf_init_threads(36, 1024 * 1024, 1, NULL);
        assert(ret == YF_OK);
        
        yf_log_file_init(NULL);
        yf_log_file_init_ctx_t log_file_init = {1024*128, 1024*1024*64, 8, 
                        "log/coroutine.log", "%t [%f:%l]-[%v]-[%r]"};

        _log = yf_log_open(YF_LOG_DEBUG, 8192, (void*)&log_file_init);
        
        _mem_pool = yf_create_pool(102400, _log);

        yf_init_bit_indexs();

        yf_init_time(_log);
        yf_update_time(NULL, NULL, _log);

        ret = yf_strerror_init();
        assert(ret == YF_OK);

        ret = yf_save_argv(_log, argc, argv);
        assert(ret == YF_OK);

        ret = yf_init_setproctitle(_log);
        assert(ret == YF_OK);

        ret = yf_init_processs(_log);
        assert(ret == YF_OK);

        testing::InitGoogleTest(&argc, (char **)argv);
        ret = RUN_ALL_TESTS();

        yf_destroy_pool(_mem_pool);

        return ret;
}



