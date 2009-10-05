FILE(STRINGS "temp.sym" first_line LIMIT_COUNT 1)
SEPARATE_ARGUMENTS(first_line)
LIST(GET first_line 3 module_id)
LIST(GET first_line 4 module_name_pdb)
STRING(
	REGEX REPLACE "(.*)\\.pdb" "\\1.sym"
	module_name_sym "${module_name_pdb}"
)
CONFIGURE_FILE("temp.sym" "./${module_name_pdb}/${module_id}/${module_name_sym}" COPYONLY)
FILE(REMOVE "temp.sym")
