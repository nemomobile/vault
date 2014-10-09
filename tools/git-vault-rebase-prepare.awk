BEGIN {
    # expects the list of git log entries in the format %H$%d$%s
    # so fields are [hash, csv of refs, subject]
    FS="$";
    date_tag_re = /^[0-9]+-[0-9]+-[0-9]+T[0-9]+-[0-9]+-.*/;
}

# skipping unit add/remove commit - deprecated
$2 == "" && substr($3, 1, 1) ~ "[+-]" {
    print "skip", $1
}

# unit backup commit
$2 == "" && substr($3, 1, 1) == ">" {
    print "add", $1, $3;
}

$2 != "" && $3 !~ date_tag_re && $3 != "anchor" {
    print "copy", $1
}

# backup tag commit subject starts with the iso date
$2 != "" && $3 ~ date_tag_re {
    split($2, refs, ",");
    is_found=0
    for (i in refs) {
        # backup tag is '>' + transformed iso date
        if (match(refs[i], /[(]?tag: (>.+Z)[)]?/, found) != 0) {
            tag=found[1];
            print "tag", $1, tag
            is_found=1
            break
        }
    }
    # tag was removed, so this backup commit should be
    # squashed/removed. Unit commits will be squashed with the
    # following ones (if any) or removed
    if (!is_found) {
        print "old_tag", $1
    }
}

# branch should be grown starting from the anchor
$3 == "anchor" {
    print "start", $1
}

# some commit w/o any refs - for the vault this is removed backup
$2 == "" && $3 ~ date_tag_re {
    print "old_tag", $1
}
