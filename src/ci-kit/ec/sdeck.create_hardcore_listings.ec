++data create_hardcore_listings.ec \Clayton \Sys\Admin
++password password
++format mcc noaddnl notrim
++control overwrite
++input
&version 2
&trace &command on
&goto &ec_name
&-
&label create_hardcore_listings
&if [not [exists argument &1]] &then &do
  &print Syntax is:
  &print   ec copy_hardcore_sources <destination_directory>
  &quit
&end
&set DEST_DIR &1
&if [not [exists directory &(DEST_DIR)]] &then &do
  &print Directory &(DEST_DIR) does not exist.
  &quit
&end
cwd &(DEST_DIR)
lf **.(pl1 alm cds pmac rd) -library supervisor_library.source
ec create_hardcore_listings_ ([segs **])
&quit
&-
&label create_hardcore_listings_
&set SOURCE_FILE &1
&- &set SUFFIX &||[suffix &(SOURCE_FILE)]
&if [exists argument &[suffix &(SOURCE_FILE)]] &then &set SUFFIX &[suffix &(SOURCE_FILE)] &else &set SUFFIX ""
&if [not [exists argument &(SUFFIX)]] &then &goto QUIT
&if [equal &(SUFFIX) "list"] &then &goto QUIT
&goto compile_&(SUFFIX)
&-
&label compile_pmac
pmac &(SOURCE_FILE) -target dps8
&- dl &(SOURCE_FILE)
&set SOURCE_FILE &[spe &(SOURCE_FILE)]
&goto compile_pl1
&-
&label compile_pl1
pl1 &(SOURCE_FILE) -ot -list
&- dl &[spe &(SOURCE_FILE)]
&goto QUIT
&-
&label compile_alm
alm &(SOURCE_FILE) -list
&- dl &[spe &(SOURCE_FILE)]
&goto QUIT
&-
&label compile_cds
cds &1 -list
&- dl &[spe &(SOURCE_FILE)]
&goto QUIT
&-
&label compile_rd
rdc &(SOURCE_FILE)
&- dl &(SOURCE_FILE)
&set SOURCE_FILE &[spe &(SOURCE_FILE)].pl1
&goto compile_pl1
&-
&label QUIT
&quit
