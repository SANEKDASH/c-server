
FLAGS = -Wall -lc -DLOG_ON -g -DMULTITHREAD

SOURCES = main.c server/server.c content_buf/content_buf.c cxn_ctx/cxn_ctx.c dirent_node/dnt_node.c \
		  my_coro_lib/my_coro.c

EXECUTABLE = serv

all:
	$(CC) $(FLAGS) $(SOURCES) -o $(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
