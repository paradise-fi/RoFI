#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/task.h"

#include "rbwebserver.h"

#define TAG "RbWebServer"

#define WORKING_DIRECTORY "/spiffs"

#define EXTRA_DIRECTORY WORKING_DIRECTORY "/extra/"

#define LISTENQ 8 /* second argument to listen() */
#define MAXLINE 256 /* max length of a line */
#define RIO_BUFSIZE 256
#define FILENAME_SIZE 64

typedef struct {
    int rio_fd; /* descriptor for this buf */
    int rio_cnt; /* unread byte in this buf */
    char* rio_bufptr; /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

typedef struct {
    char filename[FILENAME_SIZE];
    off_t offset; /* for support Range */
    size_t end;
    uint8_t servingGzip;
} http_request;

typedef struct {
    const char* extension;
    const char* mime_type;
} mime_map;

static const mime_map meme_types[] = {
    { ".css", "text/css" },
    { ".html", "text/html" },
    { ".jpeg", "image/jpeg" },
    { ".jpg", "image/jpeg" },
    { ".ico", "image/x-icon" },
    { ".js", "application/javascript" },
    { ".png", "image/png" },
    { ".json", "application/json" },
    { NULL, NULL },
};

static const char* default_mime_type = "text/plain";

static void (*extra_path_callback)(const char *path, int out_fd) = NULL;

void rb_web_set_extra_callback(void (*callback)(const char *path, int out_fd)) {
    extra_path_callback = callback;
}

static void rio_readinitb(rio_t* rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

static ssize_t writen(int fd, void* usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nwritten = 0; /* and call write() again */
            else
                return -1; /* errorno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t* rp, char* usrbuf, size_t n) {
    int cnt;
    while (rp->rio_cnt <= 0) { /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
            sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        } else if (rp->rio_cnt == 0) /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
static ssize_t rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break; /* EOF, some data was read */
        } else
            return -1; /* error */
    }
    *bufp = 0;
    return n;
}

static const char* get_mime_type(char* filename) {
    char* dot = strrchr(filename, '.');
    if (dot) { // strrchar Locate last occurrence of character in string
        const mime_map* map = meme_types;
        while (map->extension) {
            if (strcmp(map->extension, dot) == 0) {
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}

static int open_listenfd(int port) {
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            (const void*)&optval, sizeof(int))
        < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

static void url_decode(char* src, char* dest, int max) {
    char* p = src;

    int prefix_len = snprintf(dest, FILENAME_SIZE, "%s/", WORKING_DIRECTORY);
    dest += prefix_len;
    max -= prefix_len;

    char code[3] = { 0 };
    while (*p && --max > 0) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

static int prepare_gzip(http_request* req) {
    int fd;

    if (req->servingGzip == 0)
        goto nogzip;

    const int fnlen = strlen(req->filename);
    if (fnlen < 3 || fnlen + 4 >= FILENAME_SIZE)
        goto nogzip;

    if (memcmp(req->filename + fnlen - 3, ".gz", 3) == 0)
        goto nogzip;

    memcpy(req->filename + fnlen, ".gz", 4);

    fd = open(req->filename, O_RDONLY);
    req->filename[fnlen] = 0;
    if (fd >= 0) {
        return fd;
    }

nogzip:
    req->servingGzip = 0;
    return open(req->filename, O_RDONLY);
}

static void parse_request(int fd, http_request* req) {
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    req->offset = 0;
    req->end = 0; /* default */
    req->servingGzip = 0;

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s", method, uri); /* version is not cared */
    /* read all */
    while (buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
        if (buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n') {
            sscanf(buf, "Range: bytes=%ld-%u", &req->offset, &req->end);
            // Range: [start, end]
            if (req->end != 0)
                req->end++;
        } else if (strstr(buf, "Accept-Encoding: ") == buf && strstr(buf + 17, "gzip")) {
            req->servingGzip = 1;
        }
    }
    char* filename = uri;
    if (uri[0] == '/') {
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0) {
            filename = ".";
        } else {
            for (int i = 0; i < length; ++i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(filename, req->filename, FILENAME_SIZE);
}

static void log_access(int status, struct sockaddr_in* c_addr, http_request* req) {
    ESP_LOGI(TAG, "%s:%d %d - %s", inet_ntoa(c_addr->sin_addr),
        ntohs(c_addr->sin_port), status, req->filename);
}

static void client_error(int fd, int status, char* msg, char* longmsg) {
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
        "Content-length: %u\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    writen(fd, buf, strlen(buf));
}

static ssize_t sendfile(char* buf, const size_t bufsize, int out_fd, int in_fd, off_t* offset, size_t count) {
    size_t chunk;
    ssize_t res;
    ssize_t chunk_res;

    if (*offset != 0 && lseek(in_fd, *offset, SEEK_SET) < 0) {
        ESP_LOGE(TAG, "Failed to seek for partial response: %d %s\n", errno, strerror(errno));
    }

    while (count > 0) {
        chunk = count;
        if (chunk > bufsize)
            chunk = bufsize;

        chunk_res = read(in_fd, buf, chunk);
        if (chunk_res < 0) {
            return -1;
        }

        chunk_res = write(out_fd, buf, chunk_res);
        if (chunk_res < 0) {
            return -1;
        }
        res += chunk_res;
        *offset += chunk_res;
        count -= chunk;
    }
    return res;
}

static void serve_static(int out_fd, int in_fd, http_request* req,
    size_t total_size) {
    char buf[256];
    int buf_len = 0;
    if (req->offset > 0) {
        buf_len += snprintf(buf, sizeof(buf), "HTTP/1.1 206 Partial\r\nContent-Range: bytes %ld-%u/%u\r\n",
            req->offset, req->end, total_size);
    } else {
        buf_len += snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
    }

    if (req->servingGzip) {
        buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len - 1, "Content-Encoding: gzip\r\n");
    }

    buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len - 1, "Content-Length: %lu\r\n", req->end - req->offset);

    if (strstr(req->filename, ".json")) {
        buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len - 1, "Cache-Control: no-store\r\n");
    } else {
        buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len - 1, "Cache-Control: private,max-age=259200\r\n");
    }

    buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len - 1, "Content-Type: %s\r\n\r\n", get_mime_type(req->filename));

    writen(out_fd, buf, strlen(buf));
    off_t offset = req->offset; /* copy */
    while (offset < req->end) {
        if (sendfile(buf, sizeof(buf), out_fd, in_fd, &offset, req->end - req->offset) <= 0) {
            break;
        }
        ESP_LOGD(TAG, "offset: %ld \n\n", offset);
        close(out_fd);
        break;
    }
}

static void process(int fd, struct sockaddr_in* clientaddr) {
    ESP_LOGD(TAG, "accept request, fd is %d\n", fd);
    http_request req;
    parse_request(fd, &req);

    if(strncmp(req.filename, EXTRA_DIRECTORY, sizeof(EXTRA_DIRECTORY)-1) == 0) {
        if(extra_path_callback == NULL) {
            client_error(fd, 400, "Error", "No extra_path_callback specified.");
            return;
        }

        extra_path_callback(req.filename + sizeof(EXTRA_DIRECTORY)-1, fd);
        close(fd);
        return;
    }

    struct stat sbuf;
    int status = 200;
    int ffd = prepare_gzip(&req);
    if (ffd <= 0) {
        status = 404;
        char* msg = "File not found";
        client_error(fd, status, "Not found", msg);
    } else {
        fstat(ffd, &sbuf);
        if (S_ISREG(sbuf.st_mode)) {
            if (req.end == 0) {
                req.end = sbuf.st_size;
            }
            if (req.offset > 0) {
                status = 206;
            }
            serve_static(fd, ffd, &req, sbuf.st_size);
        } else {
            status = 400;
            char* msg = "Unknow Error";
            client_error(fd, status, "Error", msg);
        }
        close(ffd);
    }
    log_access(status, clientaddr, &req);
}

static void tiny_web_task(void* portPtr) {
    int port = (int)portPtr;

    struct sockaddr_in clientaddr;
    int listenfd, connfd;
    socklen_t clientlen = sizeof clientaddr;

    listenfd = open_listenfd(port);
    if (listenfd > 0) {
        ESP_LOGI(TAG, "Listening on port %d", port);
    } else {
        ESP_LOGE(TAG, "failed to start: %s", strerror(errno));
        return;
    }

    if (fcntl(listenfd, F_SETFL, O_NONBLOCK) == -1) {
        ESP_LOGE(TAG, "failed to set non-blocking: %s", strerror(errno));
        close(listenfd);
        return;
    }

    while (1) {
        connfd = accept(listenfd, (SA*)&clientaddr, &clientlen);
        if (connfd >= 0) {
            process(connfd, &clientaddr);
            close(connfd);
            continue;
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            ESP_LOGE(TAG, "failed to accept: %s", strerror(errno));
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t rb_web_start(int port) {
    {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = WORKING_DIRECTORY,
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
        };

        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
            }
            return pdFAIL;
        }
    }

    TaskHandle_t task;
    xTaskCreate(&tiny_web_task, "rbctrl_web", 3072, (void*)port, 2, &task);
    return task;
}

esp_err_t rb_web_add_file(const char* filename, const char* data, size_t len) {
    char buf[256];
    int fd;
    esp_err_t res = ESP_OK;

    snprintf(buf, sizeof(buf), "%s/%s", WORKING_DIRECTORY, filename);

    fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        return ESP_FAIL;
    }

    if (writen(fd, (void*)data, len) < 0) {
        res = ESP_FAIL;
    }

    close(fd);
    return res;
}
