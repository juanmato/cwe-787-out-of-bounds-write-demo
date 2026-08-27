# Demo CWE-787 en Docker (alternativa multiplataforma al .exe).
# Compila el servidor vulnerable y lo corre. El codigo fuente queda a la vista.
#
#   docker build -t cwe787-demo .
#   docker run --rm -p 8000:8000 cwe787-demo
#   -> abrir http://localhost:8000
FROM gcc:14-bookworm

WORKDIR /app
COPY techstore.c page.h ./

# -fno-stack-protector y -D_FORTIFY_SOURCE=0: se desactivan A PROPOSITO las
# protecciones del compilador para mostrar el CWE-787 "crudo".
# -DBIND_ANY: dentro del contenedor hay que escuchar en 0.0.0.0 (no solo localhost).
RUN gcc -O0 -fno-stack-protector -D_FORTIFY_SOURCE=0 -DBIND_ANY \
        -Wno-deprecated-declarations -o techstore techstore.c

EXPOSE 8000
CMD ["./techstore"]
