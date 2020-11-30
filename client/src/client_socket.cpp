#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "client_socket.h"

#define MAX_BUF_LEN 1024

IpcClientBase::IpcClientBase()
{
    m_efd_thread = eventfd(0, 0);
    if (m_efd_thread < 0) {
        perror("[cli] efd_thread init failed: ");
    }
    is_running_thread = true;
}
void IpcClientBase::setCB(RecvCB cb)
{
    m_recv_cb = cb;
}

IpcClientBase::~IpcClientBase()
{

}

int IpcClientBase::connectServer(string ip, int port)
{
    m_ip = ip;
    m_port = port;

    printf("[cli] connect %s:%d\n", m_ip.c_str(), m_port);
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0) {
        perror("[cli] socket error");
        return -1;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, m_ip.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (connect(m_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("[cli] connect erroe");
        return -1;
    }

    thread_recv = thread(&recvLoop, this);
    return 0;
}

int IpcClientBase::send(PkgHeader *header, const char *data, int len)
{
    if (len < 0) {
        printf("data len < 0, err\n");
        return -1;
    }
    if (write(m_sockfd, header, sizeof(PkgHeader)) < 0) {
        perror("[cli] write PkgHeader failed");
    }
    if (len == 0) {
        return 0;
    }
    if (write(m_sockfd, data, len) < 0) {
        perror("[cli] write data failed");
    }
    return 0;
}

int IpcClientBase::recvLoop(IpcClientBase *pthis)
{
    char buf_recv[MAX_BUF_LEN] = {0};

    int sockfd = pthis->m_sockfd;
    int efd_thread = pthis->m_efd_thread;

    fd_set allset;
    fd_set fdset;
    FD_ZERO(&allset);
    FD_ZERO(&fdset);
    FD_SET(sockfd, &allset);
    FD_SET(efd_thread, &allset);

    int max_fd = sockfd > efd_thread ? sockfd : efd_thread;

    while (pthis->is_running_thread == true) {
        fdset = allset;
        int nready = select(max_fd + 1, &fdset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("[cli] select error");
            break;
        }

        if (FD_ISSET(sockfd, &fdset)) {
            int n = read(sockfd, buf_recv, MAX_BUF_LEN);
            if (n == 0) {
                printf("[cli] the other side has been closed.\n");
                break;
            }
            else {
                pthis->m_mem_pool.add_block(sockfd, BLOCK_SIZE); //notice [add_block] in mem_pool
                pthis->m_mem_pool.push(sockfd, buf_recv, n);
                //pthis->m_mem_pool.show_mem_pool_status();

                while (1) {
                    MemPool::BlockDesc block = pthis->m_mem_pool.get_block_desc(0);
                    PkgHeader msg;
                    memset(&msg, 0, sizeof(PkgHeader));
                    if (block.use >= (int)sizeof(PkgHeader)) {
                        pthis->m_mem_pool.read_data(block.id, (char *)&msg, sizeof(PkgHeader));
                        //print_header(&msg);
                    }
                    else break;

                    int len_total = (int)sizeof(PkgHeader) + msg.length;
                    if (block.use >= len_total) {
                        char *tmp = new char[len_total]; // TODO 优化
                        memset(tmp, 0, len_total);
                        pthis->m_mem_pool.pop(block.id, tmp, len_total);
                        pthis->m_recv_cb(tmp, len_total);
                        delete []tmp;
                    }
                    else break;
                }
            }
        }
        if (FD_ISSET(efd_thread, &fdset)) {
            printf("[cli] select efd, to exit thread\n");
            uint64_t u = 0;
            ssize_t s = read(efd_thread, &u, sizeof(uint64_t));
            if (s != sizeof(uint64_t))
                perror("[cli] read");
            else printf("[cli] read %llu efd_thread, to break\n",(unsigned long long)u);
        }
    }
    printf("[cli] thread exit!\n");
    return 0;
}

int IpcClientBase::disConnect()
{
    is_running_thread = false;

    uint64_t u = 10;
    ssize_t s = write(m_efd_thread, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t))
        perror("[cli] m_efd_thread write");

    if (thread_recv.joinable()) {
        thread_recv.join();
    }
    usleep(500000); //wait server to recv close info
    close(m_sockfd);
    return 0;
}

