BEGIN {
    if (out_prefix == "") {
        print "provide out_prefix path" > "/dev/stderr"
        exit 1
    }
    blob_out = out_prefix ".blob.objects";
    hash_out = out_prefix ".objects";
    full_out = out_prefix ".revlist";
    print > blob_out;
    print > hash_out;
    print > full_out;
}

$2 ~ /^[A-Za-z0-9_]+\/blobs\// {
    if (! skip_blob || $2 !~ skip_blob) {
        print $1 >> blob_out
    } else {
        print "skip " $2 > "/dev/stderr"
    }
}

$1 ~ /[0-9a-f]+/ {
    git cat-file -t $1
    print $1  >> hash_out
}

{
    print $0 >> full_out
}
