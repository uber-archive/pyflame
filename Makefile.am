SUBDIRS = src
EXTRA_DIST = autogen.sh LICENSE README.md
ACLOCAL_AMFLAGS = -I m4

man1_MANS = pyflame.man

clean-local:
	rm -f core.* pyflame

test:
	bash runtests.sh
