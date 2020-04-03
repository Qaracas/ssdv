EXT = leeflujo conector

.PHONY: subdirs $(EXT)

subdirs: $(EXT)

$(EXT):
	$(MAKE) -C $@

limpia:
	$(foreach ext,$(EXT), $(MAKE) -C $(ext) limpia ;)
