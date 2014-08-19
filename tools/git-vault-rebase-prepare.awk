BEGIN {
    FS="$";
    split("", commands);
    date_tag_re = /^[0-9]+-[0-9]+-[0-9]+T[0-9]+-[0-9]+-.*/;
}

$2 == "" && substr($3, 1, 1) ~ "[+-]" {
    print "skip", $1
}

$2 == "" && substr($3, 1, 1) == ">" {
    print "add", $1, $3;
}

$2 != "" && $3 !~ date_tag_re && $3 != "anchor" {
    print "copy", $1
}

$2 != "" && $3 ~ date_tag_re {
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

$2 == "" && $3 ~ date_tag_re {
    print "old_tag", $1
}
