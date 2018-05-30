#!/bin/bash

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

SYSCOIND=${SYSCOIND:-$BINDIR/syscoind}
SYSCOINCLI=${SYSCOINCLI:-$BINDIR/syscoin-cli}
SYSCOINTX=${SYSCOINTX:-$BINDIR/syscoin-tx}
SYSCOINQT=${SYSCOINQT:-$BINDIR/qt/syscoin-qt}

[ ! -x $SYSCOIND ] && echo "$SYSCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
BTCVER=($($SYSCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for syscoind if --version-string is not set,
# but has different outcomes for syscoin-qt and syscoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$SYSCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $SYSCOIND $SYSCOINCLI $SYSCOINTX $SYSCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${BTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${BTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m