poll:
a simple read write test.

stress:
a stress test.

qfsize:
a modified stress test that will but the "queue free size" protocol to test.

openclose:
a open/close io test.

mctrl:
a program that puts GETMODEMSTATUS {CLR/SET}{RTS/DTR} SETWATIMASK WAITONMASK to test.

timeout:
a modified "mctrl", that will test the timeout mechanism.
run the program with wireshark or the debug message enabled on the daemon.
unplug the ethernet cable from the EKI device and see if TCP connection between the daemon and the EKI device breaks after 10-20 seconds.

reconn:
a modified "pull" plug and unplug the ethernet cable durring the test repeatedly.
this test will show the longest time needed to recover from a interrupted TCP connection.
