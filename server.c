#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
//#include <openssl/ssl.h>

const char *get_cmd = "GET";
const char *post_cmd = "POST";
/*SSL_CTX *ctx;
SSL *ssl;*/


void split(char *buf, char *tmp) {
    int i = 0;
    for(;buf[i] != '\"'; i++) {
        tmp[i] = buf[i];
    }
    strcat(tmp, "</br>");
    i += 5;
    int k = i + 2;
    for(; buf[k] != '\"'; i++) {
        tmp[i] = buf[k];
	k++;
    }
    strcat(tmp, "</br>\r\n\0");
}

void read_database(char *text) {
    fprintf(stderr, "test database\n");
    FILE *database = fopen("Text/text.txt", "r");
    char buf[4][512];
    int num = 0;
    int n = 4;
    while(fgets(buf[num], 512, database) != NULL) {
        num = (num  + 1) % 4;
        if(n != 0)
		n -= 1;
    }
    num = (num + n) % 4;
    for(; n != 4; n++) {
        char tmp[512];
	memset(tmp, 0, 512);
        split(&buf[num][6], tmp);
        strcat(text, tmp);
        num = (num + 1) % 4;
    }
}
void read_file(char *text, char *filename) {
    char buf[512];
    memset(buf, 0, sizeof(buf));
    FILE *headerfd = fopen(filename, "r");
    fprintf(stderr, "open %s\nsuccess\n", filename);
    while(fgets(buf, 512, headerfd) != NULL) {
        //strcat(buf, "\r\n\0");
        strcat(text, buf);
	//fprintf(stderr, "%s %d\n\n", buf, strlen(buf));
        if(strncmp(buf,"    <div id=\"text\">", 16) == 0) {
            read_database(text);
        }
    }
    //strcat(text, "\r\n");
    fprintf(stderr, "%s\nsuccess\n", filename);
    fclose(headerfd);
}

void write_text(char *text, char *filename) {
    FILE *wdfd = fopen(filename, "ab");
    int n = strlen(text);
    for(int i = 0; i < n; i++) {
	int c = text[i];

	fputc(c, wdfd);
    }
    fclose(wdfd);
}


void sendhtml(int newsock) {
    char text[10000];
    memset(text, 0, sizeof(text));
    read_file(text, "header");
    strcat(text, "\r\n");
    read_file(text, "webpage.html");
    int k;
    if((k = send(newsock, (const void *)text, strlen(text), 0)) < 0) {
        fprintf(stderr, "can't send\n");
        return;
    }
    printf("send %s\n\n", text);
}

int read_img(char *text, char *filename) {
    FILE *imgfd = fopen(filename, "rb");
    //FILE *textfd = fopen("test.png", "w");
    int c;
    int n = strlen(text);
    while((c = fgetc(imgfd)) != EOF) {
        text[n] = c;
        text[n + 1] = '\0';
        n++;
        //fputc(c, textfd);
        //printf("%02x ", c);
    }
    printf("\n");
    fclose(imgfd);
    //fclose(textfd);
    return n;
}

void sendimg(int newsock, char *s) {
    char text[50000];
    memset(text, 0, sizeof(text));
    read_file(text,"img_head");
    text[strlen(text) - 1] = '\0';
    strcat(text, s);
    strcat(text, "\r\n\n");
    int n = read_img(text, s);
    printf("n = %d\n", n);
    int k;
    if((k = send(newsock, (const void *)text, n, 0)) < 0) {
        fprintf(stderr, "can't send img\n");
        return;
    }
    printf("send %d %ld\n%s\n", k, strlen(text), text);
}

void sendvid(int newsock, char *s) {
    char *text = (char *)malloc(100000000);
    memset(text, 0, sizeof(text));
    read_file(text, "video_head");
    text[strlen(text) - 1] = '\0';
    strcat(text, s);
    strcat(text, "\r\n\n");
    int n = read_img(text, s);
    int k;
    if((k = send(newsock, (const void *)text, n, 0)) < 0) {
        fprintf(stderr, "can't send video\n");
        return;
    }
    fprintf(stderr, "send %d %ld\n%s\n", k, strlen(text), text);
    free(text);
}

void write_to_text(int newsock, char* buf, char *filename) {
    buf = strtok(buf, "\n");
    bool istext = false;
    while(buf != NULL) {
	if(istext && buf[0] != 13) {
		write_text(buf, filename);
		write_text("\n", filename);
		fprintf(stderr, "write string %s %d\n", filename, buf[0]);
	}
	if(strncmp(buf, "Accept-Language", 15) == 0)
		istext = true;
        buf = strtok(NULL, "\n");
    }
    char response[512];
    memset(response, 0, sizeof(response));
    read_file(response, "post_head");
    strcat(response, "\r\n");
    int k;
    if((k = send(newsock, (const void *)response, strlen(response), 0)) < 0) {
        fprintf(stderr, "can't send response\n");
        return;
    }
    printf("send\n%s\n", response);
}

void get(int newsock, char *buf) {
    char s[512];
    sscanf(buf, "%s", s);
    if(strcmp(s, "/") == 0) {
        sendhtml(newsock);
    }
    else if(strncmp(s, "/Image", 6) == 0){
        sendimg(newsock, &s[1]);
    }
    else if(strncmp(s, "/Video", 6) == 0) {
        sendvid(newsock, &s[1]);
    }
}

void post(int newsock, char *buf) {
    char s[512];
    printf("handle post\n");
    sscanf(buf, "%s", s);
    if(strncmp(s, "/Text", 5) == 0) {
	printf("try to write\n");
        write_to_text(newsock, buf, &s[1]);
    }
}

/*void ssl_init() {
    SSL_library_init();
    ctx = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_set_ecdh_auto(ctx, 1);
    SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM);
    if(!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "key wrong\n");
        exit(0);
    }

}*/

int main() {
    //ssl_init();
    int port_num = 7891;
    struct sockaddr_in sock_addr;
    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        fprintf(stderr, "can't create sockfd\n");
        exit(0);
    }
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port_num);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(sock_addr.sin_zero, '\0', sizeof(sock_addr.sin_zero));
    int reuse = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("set error\n");
        exit(0);
    }


    if(bind(sockfd, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "can't bind socket\n");
        close(sockfd);
        exit(0);
    }
    else {
        printf("binding..\n");
    }
    char hostname[64];
    struct hostent *hent;
    if(gethostname(hostname, sizeof(hostname)) < 0) {
        printf("get name error\n");
    	exit(0);
    }

    if((hent = gethostbyname(hostname)) == NULL) {
        printf("get hostent error\n");
        exit(0);
    }
    printf("host %s\n", hent->h_name);
    for(int i = 0; hent->h_addr_list[i]; i++) {
        printf("%s:%d\n", inet_ntoa(*(struct in_addr*)(hent->h_addr_list[i])), port_num);
    }
    if(listen(sockfd, 10) == -1) {
        fprintf(stderr, "can't listen socket\n");
        close(sockfd);
        exit(0);
    }
    else {
        printf("listen..\n");
    }

    while(1) {
        printf("try accept..\n");
        int newsock = accept(sockfd, NULL, NULL);
        if(newsock < 0) {
            fprintf(stderr, "can't accept\n");
            close(sockfd);
            exit(0);
        }
        else {
            printf("accept\n");
            /*ssl = SSL_new(ctx);
            SSL_set_fd(ssl, newsock);
            int acc = SSL_accept(ssl);
            if(acc < 0) {
                fprintf(stderr, "error");
                exit(0);
            }*/
        }
        char buf[4096];
        int n;
        while((n = recv(newsock, (void *)buf, sizeof(buf), 0)) != 0) {
            if(n < 0) {
                fprintf(stderr, "can't recv\n");
                break;
            }
            else if(n > 0) {
                buf[n] = '\0';
                fprintf(stderr, "%d %s\n", n, buf);
            }
            if(strncmp(buf, get_cmd, 3) == 0) {
                get(newsock, &buf[3]);
                break;
            }
            else if(strncmp(buf, post_cmd, 4) == 0) {
                post(newsock, &buf[4]);
                break;
            }
        }
        shutdown(newsock, SHUT_RDWR);
        //SSL_free(ssl);
        close(newsock);
    }
    //SSL_CTX_free(ctx);
}


