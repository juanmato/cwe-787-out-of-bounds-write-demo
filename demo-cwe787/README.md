# Demo CWE-787 — Out-of-bounds Write (Tienda online "TechStore")

Instructivo para reproducir la demo (entregable III).

Un checkout de **TechStore** que por detrás corre un programa en **C** con un bug clásico
de **escritura fuera de límites**: el buffer del cupón `coupon[16]` y el descuento
`discount_percent` son campos **contiguos** en memoria, y la copia con `strcpy()` **no
controla la longitud**. Un cupón de más de 16 bytes **desborda el buffer y sobre-escribe el
descuento** con un número gigante → el total queda en **$0** y te llevás la notebook **gratis**,
sin un cupón válido.

---

## ▶️ Correr con Docker (recomendado)

Requiere Docker instalado y corriendo. Compila el servidor en C dentro del contenedor y lo sirve;
el código fuente queda a la vista y no se distribuye ningún binario precompilado.

```bash
cd demo-cwe787
docker build -t cwe787-demo .
docker run --rm -p 8000:8000 cwe787-demo
```

Abrí **http://localhost:8000**. Para **detener**: `Ctrl+C` en la terminal.

## ▶️ Build nativo (sin Docker, mac/Linux)

Si preferís no usar Docker, podés compilar el servidor autocontenido con `gcc` o `clang`:

```bash
cd demo-cwe787
gcc -O0 -fno-stack-protector -D_FORTIFY_SOURCE=0 -Wno-deprecated-declarations \
    -o techstore techstore.c
./techstore            # abre http://localhost:8000
```

> Alternativa: `./build.sh` compila las versiones mínimas `bin/vuln` y `bin/secure`, y
> `python3 server.py` levanta un servidor que las invoca. Útil para inspeccionar cada versión por
> separado.

---

## 🎬 Guion de la demo en vivo

Cupones legítimos de la tienda: **`SALE10`** (10%) y **`SALE25`** (25%).

1. **Backend VULNERABLE** (por defecto):
   - Cupón `DESCUENTO` (inválido) → se cobra **$1.200** (precio completo, normal).
   - Cupón `SALE25` (válido) → **$900** (25% de descuento legítimo).
   - Botón **💥 Cupón malicioso** (20 letras "A") → **Total a pagar: GRATIS**.
     Mirá la memoria de la derecha: los 20 bytes se llenaron de `41` ("A") y los
     **4 bytes de `discount_percent` (16–19) quedan en rojo** → el desborde pisó el descuento,
     que ahora vale 1.094.795.585 %. El cupón NO es válido, pero igual pagás $0.

2. **Cambiá a "Backend CORREGIDO"** y repetí el cupón malicioso:
   - → **rechazado / precio completo $1.200**. La copia acotada (`strncpy` + límite) impide el
     desborde; los bytes del descuento quedan intactos en `00`.

---

## 📁 Archivos

| Archivo            | Qué es |
|--------------------|--------|
| `Dockerfile`       | Compila y corre la demo en un contenedor (método recomendado). |
| `techstore.c`      | Servidor autocontenido + lógica (portable Windows/mac/Linux). Es lo que compila Docker. |
| `vuln.c`           | Versión mínima **vulnerable** (`strcpy` sin límite → CWE-787). Referencia didáctica. |
| `secure.c`         | Versión mínima **corregida** (`strncpy` acotado + `\0` + rechazo). Referencia didáctica. |
| `index.html`       | La página del checkout + visualización de memoria. |
| `page.h`           | `index.html` embebida en un header C (autogenerado por `gen_page.py`). |
| `server.py`        | Servidor alternativo en Python que usa `bin/vuln` y `bin/secure`. |
| `build.sh` / `gen_page.py` | Compilar `vuln`/`secure` y regenerar `page.h`. |

---

## 🧠 Notas para el que pregunte en clase

- **Se desactivan protecciones a propósito** (`-fno-stack-protector -D_FORTIFY_SOURCE=0`) para
  mostrar el CWE-787 "crudo". En un binario endurecido, el *stack canary* y `FORTIFY_SOURCE`
  **abortan el programa** ante este overflow — pero son mitigaciones, no el arreglo: el bug sigue
  y hay que corregir el código.
- El `_pad[64]` de la struct es solo para que **la demo no crashee** al proyectar: absorbe el
  desborde para que no toque el marco de pila real.
- Dentro del contenedor el servidor escucha en `0.0.0.0:8000` (necesario para publicar el puerto);
  el cupón se procesa en el proceso en C y **no hay inyección de comandos** ni exposición extra:
  lo único vulnerable es el desborde en C, que es justo lo que queremos demostrar.
- ¿Por qué una página y no C directo? Out-of-bounds Write es un bug de **memoria nativa** (no existe
  en JavaScript). La página es solo la interfaz; el desborde real ocurre en el binario en C.
