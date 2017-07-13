SUBDIRS = src
EXTRA_DIST = autogen.sh LICENSE README.md
ACLOCAL_AMFLAGS = -I m4

man1_MANS = pyflame.man

.PHONY: clean-local
clean-local:
	rm -f core.* pyflame

.PHONY: test
test:
	./runtests.sh
