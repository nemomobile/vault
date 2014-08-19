BEGIN {
    ok=0
    split("", commits);
    split("", commands);
    split("", old_tags);
    split("", move_tags);
}

function array_clear(arr) {
    delete arr;
    split("", arr);
}

# buggy gawk 3.1.5 used in mer
# see https://lists.gnu.org/archive/html/bug-gnu-utils/2008-03/msg00029.html
# so pass also len
function push(arr, len, data) {
    arr[len + 1] = data;
}

function push_command(cmd) {
    push(commands, length(commands), cmd);
}

function prepare_commands() {
    for (name in commits) {
        split(commits[name], ids, SUBSEP);
        if (length(ids) == 1) {
            push_command("git cherry-pick " ids[1]);
        } else {
            cmd="git cherry-pick " ids[1];
            for (i = 1; i <= length(ids); i++) {
                cmd = cmd " && git cherry-pick --no-commit " ids[i] \
                    " && git commit --no-edit --amend";
            }
            push_command(cmd);
        }
    }
    array_clear(commits);

    # tags
    id = $2
    tag = $3
    if (length(old_tags) != 0) {
        for (i = 1; i <= length(old_tags); i++) {
            push_command(old_tags[i]);
        }
        array_clear(old_tags);
        push_command("git cherry-pick --no-commit " id);
        push_command("git commit --no-edit --amend");
    } else {
        push_command("git cherry-pick " id);
    }
    tmp_tag = "migrate/" tag
    push_command(sprintf("git tag -f '%s'", tmp_tag));
    push_command("(git notes copy " id " HEAD || :)");
    cmd = sprintf("(git tag -f '%s' '%s' && git tag -d '%s')", tag, tmp_tag, tmp_tag);
    push(move_tags, length(move_tags), cmd);
}

$1 == "add" {
    if (commits[$3] != "") {
        commits[$3] = commits[$3] SUBSEP $2;
    } else {
        commits[$3] = $2;
    }
}

$1 == "old_tag" {
    if (length(old_tags) == 0) {
        push(old_tags, length(old_tags), "git cherry-pick " $2);
    } else {
        push(old_tags, length(old_tags), "git cherry-pick --no-commit " $2);
        push(old_tags, length(old_tags), "git commit --no-edit --amend");
    }
}

$1 == "tag" {
    prepare_commands();
}

$1 == "copy" {
    push_command("git cherry-pick " $2);
}

$1 == "start" {
    push_command("(git checkout master && (git reset --hard && git clean -fd))");
    push_command("git checkout -b migrate " $2);
}

END {
    for (i = 1; i <= length(move_tags); i++) {
        push_command(move_tags[i]);
    }
    push_command("git branch -D master");
    push_command("git branch -m migrate master");
    res = ""
    sep = ""
    for (i = 1; i <= length(commands); i++) {
        res = res sep commands[i];
        sep = " && \\\n\t"
    }
    print res;
}
