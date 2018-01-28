#!/bin/bash
set -x

for i in ../man/*.man
do
        bname=$(basename $i .man)
#        rman -f HTML -S ../$bname.man >$bname.6.html
        man2html -r ../man/$bname.man |sed -e 's|../man6/|../conquest/|g' \
            -e 's|Content-type: text/html||g' \
            -e 's|http://localhost/cgi-bin/man/man2html|http://hydra.nac.uci.edu/indiv/ehood/man2html.html|g' >$bname.6.html
done

