sed \
  -e 's/\[White "\([^"]*\)\"\]/\1:/' \
  -e 's/\[Black "\([^"]*\)\"\]/\1:/' \
  -e 's/\[Result "1-0"\]/w/' \
  -e 's/\[Result "0-1"\]/b/' \
  -e 's/\[Result "1\/2-1\/2"\]/d/' \
  $1 | awk -v RS='\n\n' -v FS='\n' '{printf "%s%s%s\n",$1,$2,$3}'
