all: mctrl openclose poll qfsize stress reconn timeout tiocmiwait

mctrl: mctrl.c
	gcc -Wall $^ -o $@
openclose: openclose.c
	gcc -Wall $^ -o $@
poll: poll.c
	gcc -Wall $^ -o $@
qfsize: qfsize.c
	gcc -Wall $^ -o $@
stress: stress.c
	gcc -Wall $^ -o $@
reconn: reconn.c
	gcc -Wall $^ -o $@
timeout: timeout.c
	gcc -Wall $^ -o $@
tiocmiwait: tiocmiwait.c
	gcc -Wall $^ -o $@

clean:
	rm -f mctrl
	rm -f openclose
	rm -f poll
	rm -f qfsize
	rm -f stress
	rm -f reconn
	rm -f timeout
	rm -f tiocmiwait
