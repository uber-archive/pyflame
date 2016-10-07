SUBDIRS = src
EXTRA_DIST = autogen.sh LICENSE README.md
ACLOCAL_AMFLAGS = -I m4

clean-local:
	rm -f core.* pyflame

test:
	bash runtests.sh
