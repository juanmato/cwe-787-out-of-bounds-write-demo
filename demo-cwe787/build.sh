#!/usr/bin/env bash
# Compila ambas versiones del binario. Las flags -fno-stack-protector y
# -D_FORTIFY_SOURCE=0 DESACTIVAN las protecciones automaticas del compilador
# a proposito, para poder demostrar el CWE-787 "crudo" en clase.
set -e
cd "$(dirname "$0")"
mkdir -p bin
CC="${CC:-clang}"
FLAGS="-O0 -fno-stack-protector -D_FORTIFY_SOURCE=0 -Wno-deprecated-declarations"
echo "Compilando version VULNERABLE -> bin/vuln"
$CC $FLAGS -o bin/vuln  vuln.c
echo "Compilando version SEGURA     -> bin/secure"
$CC $FLAGS -o bin/secure secure.c
echo "Listo."
