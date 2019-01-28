# Creates config.mk and config.h.
add_define MAKEFILE GLOBAL_CONFIG_DIR "$GLOBAL_CONFIG_DIR"
set -- $(set | grep ^HAVE_)
while [ $# -gt 0 ]; do
	tmpvar="${1%=*}"
	shift 1
	var="${tmpvar#HAVE_}"
	vars="${vars} $var"
done
VARS="$(printf %s "$vars" | tr ' ' '\n' | $SORT)"
create_config_make config.mk $(printf %s "$VARS")
create_config_header config.h $(printf %s "$VARS")
