README.md: README.md.t src/gsc.cpp Makefile
	expand-macros.py $< $@ # requires `macro_expander` (https://github.com/CD3/macro_expander)
	sed -i "s|./build/gsc|gsc|"  $@
