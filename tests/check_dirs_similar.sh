#!/bin/sh

cd $1 2>/dev/null && find -P . | sort > /tmp/.check-similar-1
cd $2 2>/dev/null && find -P | sort > /tmp/.check-similar-2
diff /tmp/.check-similar-1 /tmp/.check-similar-2 > /tmp/.check-similar-diff
cat /tmp/.check-similar-diff

