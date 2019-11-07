/*
 * Standard I/O wrapper functions.
 */

#include	"unp.h"
static int read_cnt;//刚开始可以置为一个负值（我的理解）
static char *read_ptr;
static char read_buf[MAXLINE];
void
Fclose(FILE *fp)
{
	if (fclose(fp) != 0)
		err_sys("fclose error");
}

FILE *
Fdopen(int fd, const char *type)
{
	FILE	*fp;

	if ( (fp = fdopen(fd, type)) == NULL)
		err_sys("fdopen error");

	return(fp);
}

char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}

FILE *
Fopen(const char *filename, const char *mode)
{
	FILE	*fp;

	if ( (fp = fopen(filename, mode)) == NULL)
		err_sys("fopen error");

	return(fp);
}

void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
}

ssize_t /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ( (nread = read(fd, ptr, nleft)) < 0) 
		{
			if (errno == EINTR)
				nread = 0; /* and call read() again */
			else
				return(-1);
		} 
		else if(nread == 0)
			break; /* EOF */
		nleft -= nread;
		ptr += nread;
	}
	return(n - nleft);/* return >= 0 */
}
/* end readn */
ssize_t
Readn(int fd, void *ptr, size_t nbytes)
{
	ssize_t n;
	if ( (n = readn(fd, ptr, nbytes)) < 0)
	err_sys("readn error");
	return(n);
}

ssize_t /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0; /* and call write() again */
			else
				return(-1); /* error */
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return(n);
}
/* end writen */
void Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
	err_sys("writen error");
}

static ssize_t
my_read(int fd, char *ptr)//每次最多读取MAXLINE个字符，调用一次，每次只返回一个字符
{
if (read_cnt <= 0) {
again:
if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {//如果读取成功，返回read_cnt=读取的字符if (errno == EINTR)
goto again;
return(-1);
} else if (read_cnt == 0)
return(0);
read_ptr = read_buf;
}
read_cnt--;//每次递减1，直到<0读完，才执行上面if的命令。
*ptr = *read_ptr++;//每次读取一个字符，转移一个字符
return(1);
}
ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;
	ptr = (char*)vptr;
	for (n = 1; n < maxlen; n++) 
	{
		if ( (rc = my_read(fd, &c)) == 1) 
		{
			*ptr++ = c;
			if (c == '\n')
				break; /* newline is stored, like fgets() */
		} 
		else if (rc == 0) 
		{
			*ptr = 0;
			return(n - 1);/* EOF, n - 1 bytes were read */
		} 
		else
			return(-1); /* error, errno set by read() */
	}
	*ptr = 0; /* null terminate like fgets() */
	return(n);
}
ssize_t
readlinebuf(void **vptrptr)
{
	if (read_cnt)
		*vptrptr = read_ptr;
	return(read_cnt);
}
/* end readline */

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t n;
	if ( (n = readline(fd, ptr, maxlen)) < 0)
		err_sys("readline error");
	return(n);
}
