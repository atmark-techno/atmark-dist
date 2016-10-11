#
# convert mac address to SMFv2 SA Code
#   input: 00:hh:ii:jj:kk:ll
#   output: 02HH-IIFF-FEJJ-KKLL
#
# XXX: assume first byte is "00"
#
# usage:
#   (linux)
#   ip link show eth0 | awk '/ether/ {print $2;}' | awk -f mac2sacode.awk
#

BEGIN {
    FS=":";
}

{
    if (NF != 6 || NR != 1) {
        print "invalid format: " $0
        exit 1
    }
    s = sprintf("%s%s-%sFF-FE%s-%s%s", "02", $2, $3, $4, $5, $6);
    print toupper(s);
}
