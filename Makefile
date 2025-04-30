FLAGS = -Wall -lc -DLOG_ON

SOURCES = main.c server/server.c content_buf/content_buf.c

EXECUTABLE = serv

all:
	$(CC) $(FLAGS) $(SOURCES) -o $(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)
