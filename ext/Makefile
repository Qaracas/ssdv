EXT = leeflujo conector

.PHONY: subdirs $(EXT)

subdirs: $(EXT)

$(EXT):
	$(MAKE) -C $@

limpia:
	$(foreach i,$(EXT),$(MAKE) -C $(i) limpia;)
