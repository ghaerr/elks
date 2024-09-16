BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

# TODO: write	# Do not compile
PRGS=basename clear date dirname echo false printenv pwd true which whoami \
	yes logname tr xargs mesg stty test uname

all: $(PRGS)

write: write.o ../sys_utils/utent.o

basename: basename.o
	$(LD) $(LDFLAGS) -o basename basename.o $(LDLIBS)

clear: clear.o
	$(LD) $(LDFLAGS) -o clear clear.o $(LDLIBS)

date: date.o
	$(LD) $(LDFLAGS) -o date date.o $(LDLIBS)

dirname: dirname.o
	$(LD) $(LDFLAGS) -o dirname dirname.o $(LDLIBS)

#echo: LDFLAGS += -maout-symtab
echo: echo.o
	$(LD) $(LDFLAGS) -o echo echo.o $(LDLIBS)

false: false.o
	$(LD) $(LDFLAGS) -o false false.o $(LDLIBS)

printenv: printenv.o
	$(LD) $(LDFLAGS) -o printenv printenv.o $(LDLIBS)

pwd: pwd.o
	$(LD) $(LDFLAGS) -o pwd pwd.o $(LDLIBS)

true: true.o
	$(LD) $(LDFLAGS) -o true true.o $(LDLIBS)

which: which.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o which which.o $(TINYPRINTF) $(LDLIBS)

whoami: whoami.o
	$(LD) $(LDFLAGS) -o whoami whoami.o $(LDLIBS)

yes: yes.o
	$(LD) $(LDFLAGS) -o yes yes.o $(LDLIBS)

logname: logname.o
	$(LD) $(LDFLAGS) -o logname logname.o $(LDLIBS)

tr: tr.o
	$(LD) $(LDFLAGS) -o tr tr.o $(LDLIBS)

xargs: xargs.o
	$(LD) $(LDFLAGS) -o xargs xargs.o $(LDLIBS)

mesg: mesg.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o mesg mesg.o $(TINYPRINTF) $(LDLIBS)

stty: stty.o
	$(LD) $(LDFLAGS) -o stty stty.o $(LDLIBS)

test: test.o
	$(LD) $(LDFLAGS) -o test test.o $(LDLIBS)

uname: uname.o
	$(LD) $(LDFLAGS) -o uname uname.o $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) *.o $(PRGS)
