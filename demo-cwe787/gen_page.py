#!/usr/bin/env python3
# Embebe index.html dentro de un header C (page.h) para que el .exe sea autocontenido.
import os
here = os.path.dirname(os.path.abspath(__file__))
data = open(os.path.join(here, "index.html"), "rb").read()
with open(os.path.join(here, "page.h"), "w") as f:
    f.write("/* Autogenerado por gen_page.py — NO editar a mano. */\n")
    f.write(f"static const unsigned int PAGE_LEN = {len(data)};\n")
    f.write("static const unsigned char PAGE[] = {")
    f.write(",".join(str(b) for b in data))
    f.write("};\n")
print(f"page.h generado ({len(data)} bytes)")
