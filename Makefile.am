SUBDIRS = src
EXTRA_DIST = autogen.sh LICENSE README.md
ACLOCAL_AMFLAGS = -I m4

man1_MANS = docs/pyflame.man

.PHONY: clean-local
clean-local:
	rm -f core.* pyflame

.PHONY: check
check:
	./runtests.sh -v
