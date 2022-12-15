SDIR    =./src
IDIR    =./include
ODIR    =./obj
FDIR  	=./files

CC      =gcc
CFLAGS  =-Wall -Wextra -g -I$(IDIR)

all: setup
 
setup: $(ODIR)/app.o $(ODIR)/URL.o $(ODIR)/clientTCP.o
	mkdir $(FDIR)
	$(CC) $(ODIR)/app.o $(ODIR)/URL.o $(ODIR)/clientTCP.o $(CFLAGS) -o $(FDIR)/download

$(ODIR)/%.o: $(SDIR)/%.c | $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

$(ODIR):
	mkdir $(ODIR)

download:
	rm -rf obj
	cd $(FDIR); \
	./download ftp://netlab1.fe.up.pt/pub.txt; \
	./download ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt; \
	./download ftp://rcom:rcom@netlab1.fe.up.pt/files/pic1.jpg; \
	./download ftp://rcom:rcom@netlab1.fe.up.pt/files/pic2.png; \
	# ./download ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4; \
	rm -f download; \

clean: 
	rm -f download
	rm -rf obj files
