BEGIN {
    FS="$";
    split("", commands);
}

$2 == "" && substr($3, 1, 1) ~ "[+-]" {
}

$2 == "" && substr($3, 1, 1) == ">" {
    print "add", $1, $3;
}

$2 != "" {
    split($2, refs, ",");
    for (i in refs) {
        if (match(refs[i], /[(]?tag: (>.+Z)[)]?/, found) != 0) {
            tag=found[1];
            print "tag", $1, tag
        }
    }
}

$3 == "anchor" {
    print "start", $1
}

$2 == "" && $3 ~ /^[0-9]+-[0-9]+-[0-9]+T[0-9]+-[0-9]+-.*/ {
    print "old_tag", $1
}
