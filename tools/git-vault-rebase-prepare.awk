BEGIN {
    FS="$";
    split("", commands);
}

$2 == "" && substr($3, 1, 1) ~ "[+-]" {
}

$2 == "" && substr($3, 1, 1) == ">" {
    print "add", $1, $3;
}

$3 != "anchor" && $2 != "" {
    split($2, refs, ",");
    is_found=0
    for (i in refs) {
        if (match(refs[i], /[(]?tag: (>.+Z)[)]?/, found) != 0) {
            tag=found[1];
            print "tag", $1, tag
            is_found=1
            break
        }
    }
    if (!is_found) {
        print "old_tag", $1
    }
}

$3 == "anchor" {
    print "start", $1
}

$2 == "" && $3 ~ /^[0-9]+-[0-9]+-[0-9]+T[0-9]+-[0-9]+-.*/ {
    print "old_tag", $1
}
