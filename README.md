# pid-tools
Some commandline tools for dealing with pids and their relations.

Compatible with linux/unix systems and (probably?)cygwin.


Programs
========


getppid
-------

The **getppids** tool takes a single pid (process-ID) as a commandline argument, and prints the PID of the parent process.


*Example:*

Here, I'm running bash within screen. $$ = current shell pid, so the parent pid should be of my screen program.

	[pid-tools]$ getppid $$   # 
	2137

And to confirm:

	[pid-tools]$ cat "/proc/`getppid $$`/cmdline"
	SCREEN


getcpids
--------

The **getcpids** tool takes a single pid (process-ID) as a commandline argument, and prints the PIDs of all children, if any.

It only prints the direct children, not any of their children (i.e. not recursive).


*Example:*

Extension to the getppid example, I now check for all the child pids of my "screen" process.

I have 7 windows/screens open, so I should get 7 direct children.


	[pid-tools]$ getcpids 2137
	1718 2138 2968 3443 10558 23791 26054


And to verify, first show that our current shell shows up:

	[pid-tools]$ echo $$
	1718

Now, check every one of the children of screen. Walk back to the parent, and assert that they all match:

	[pid-tools]$ for cpid in $(getcpids `getppid $$`); do [ "`getppid ${cpid}`" != "2137" ] && echo "Child ${cpid} does not have expected parent."; done
	[pid-tools]$ 


Installation
============


Using git, checkout the code:

	git clone https://github.com/kata198/pid-tools

cd into that directory,

	cd pid-tools

Run "make" to build code. Set the **CFLAGS** variable to use your own custom flags, or uncomment the top **CFLAGS** entry in "Makefile".

	make

Now, run "make install" or "sudo make install" to install in default location.

If you can write to /usr/bin, it will attempt to install there. Otherwise, it will attempt to install in $HOME/bin.

If you'd like to install at a different root, like "/usr/local" for example, set either **PREFIX** or **DESTDIR** when running install.

	make install DESTDIR=/usr/local

You may need to run "hash -r" after installing, depending on your shell, to refresh that these items are now in **PATH**.


Enjoy! <3 <3 <3
