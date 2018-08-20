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


getpcmd
-------

getpcmd returns the command-line string for a given pid (i.e. what was used to execute it, program name and arguments).

Takes an optional arg: "\-\-quoted" which will quote (and properly escape) all entries in the output.


Example:


	[pid-tools]$ getpcmd "`getppid`"

	screen -s -/bin/bash


getpenv
-------

getpenv fetches the value of an environment variable from an arbitrary pid.

Example:

	[pid-tools]$ getpenv 12345 PATH
	/home/blah/bin:/sbin:/usr/bin:/usr/local/bin:/opt/citrix/bin


getpmem
-------

getpmem fetches memory information on one or more pids

Example:

	[pid-tools]$ getpmem 365 1660

	Memory info for pid: 365 ( gmain )
	----------------------------------------
	RssAnon:            1704 kB
	RssFile:            3844 kB
	RssShmem:              0 kB
	VmRSS:              5548 kB
	========================================

	Memory info for pid: 1660 ( bash )
	----------------------------------------
	RssAnon:          332376 kB
	RssFile:          102088 kB
	RssShmem:           2120 kB
	VmRSS:            436584 kB
	========================================


See getpmem \`--help' for output options, including various units alternate to kB.


isaparentof
----------

Determines if a given pid is a parent (of any level) to another pid (i.e. direct parent, parent-of-parent, etc. all the way up).

To see if something is explicitly a direct parent, use getppid.

Read like "${first arg} is a parent of {second arg}"

Example:

	[pid-tools]$ isaparentof 15434 211 && echo "yes"


isachildof
----------

Determine if a given pid is a child (of any level) to another pid (i.e. direct child, child-of-child, etc. all the way down).

To see if something is explicitly a direct child, compare child pid to \`getppid ${child}\`

Example:

	[pid-tools]$ isachildof 211 15434 && echo "yes"


waitpid
-------

Wait for a given pid to complete.

Unlike the "wait" shell builtin, this allows you to wait on pids without the requirement that they be children of the current process.

Example:

	[pid-tools]$ waitpid `pidof somejob.sh` && ./nextjob.sh


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
