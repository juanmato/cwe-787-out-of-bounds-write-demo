# Demo CWE-787 — Out-of-bounds Write (Tienda online "TechStore")

Instructivo para reproducir la demo (entregable III).

Un checkout de **TechStore** que por detrás corre un programa en **C** con un bug clásico
de **escritura fuera de límites**: el buffer del cupón `coupon[16]` y el descuento
`discount_percent` son campos **contiguos** en memoria, y la copia con `strcpy()` **no
controla la longitud**. Un cupón de más de 16 bytes **desborda el buffer y sobre-escribe el
descuento** con un número gigante → el total queda en **$0** y te llevás la notebook **gratis**,
sin un cupón válido.

---

## ▶️ Windows 11 — un solo clic (recomendado, no instala nada)

1. Copiá la carpeta a la máquina (o descomprimí el `.zip`).
2. Doble clic en **`INICIAR-DEMO.bat`**.
   - Se abre una ventana negra (el servidor) y el navegador en **http://localhost:8000**.
   - Si Windows muestra *"Windows protegió su PC"* (SmartScreen), clic en **Más información →
     Ejecutar de todas formas**. Es porque el `.exe` no está firmado; el código fuente está incluido.
   - No pide permiso de Firewall: el servidor escucha **solo en `localhost` (127.0.0.1)**.
3. Para **detener**: cerrá la ventana negra titulada *"TechStore - Demo CWE-787"*.

> Si el `.bat` no abre el navegador, abrilo a mano y entrá a `http://localhost:8000`.
> También podés hacer doble clic directo en `techstore.exe` y abrir esa URL vos.

`techstore.exe` es **autocontenido**: incluye la página y la lógica. No necesita Python,
ni compilador, ni conexión a internet.

## ▶️ macOS / Linux

```bash
cd demo-cwe787
./build.sh            # compila las versiones nativas
./bin/techstore       # abre http://localhost:8000
```

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
| `INICIAR-DEMO.bat` | Lanzador de un clic para Windows. |
| `techstore.exe`    | **Demo autocontenida para Windows** (servidor + página + lógica). |
| `techstore.c`      | Fuente del servidor + lógica (portable Windows/mac/Linux). |
| `vuln.c` / `vuln.exe`     | Versión mínima **vulnerable** (`strcpy` sin límite → CWE-787). |
| `secure.c` / `secure.exe` | Versión mínima **corregida** (`strncpy` acotado + `\0` + rechazo). |
| `index.html`       | La página del checkout + visualización de memoria. |
| `server.py`        | Servidor alternativo en Python (mac/Linux, usa `bin/vuln` y `bin/secure`). |
| `build.sh` / `gen_page.py` | Compilar en mac/Linux y regenerar la página embebida. |

## 🔧 Recompilar el `.exe` de Windows (opcional)

Necesitás un compilador de C para Windows (por ej. **MinGW-w64**):

```bash
python3 gen_page.py    # regenera page.h desde index.html
x86_64-w64-mingw32-gcc -O0 -fno-stack-protector -D_FORTIFY_SOURCE=0 \
    -static -o techstore.exe techstore.c -lws2_32 -lwsock32
```

---

## 🧠 Notas para el que pregunte en clase

- **Se desactivan protecciones a propósito** (`-fno-stack-protector -D_FORTIFY_SOURCE=0`) para
  mostrar el CWE-787 "crudo". En un binario endurecido, el *stack canary* y `FORTIFY_SOURCE`
  **abortan el programa** ante este overflow — pero son mitigaciones, no el arreglo: el bug sigue
  y hay que corregir el código.
- El `_pad[64]` de la struct es solo para que **la demo no crashee** al proyectar: absorbe el
  desborde para que no toque el marco de pila real.
- El servidor escucha **solo en `127.0.0.1`** y el cupón se procesa en el proceso en C; no hay
  inyección de comandos ni exposición a la red: lo único vulnerable es el desborde en C, que es
  justo lo que queremos demostrar.
- ¿Por qué una página y no C directo? Out-of-bounds Write es un bug de **memoria nativa** (no existe
  en JavaScript). La página es solo la interfaz; el desborde real ocurre en el binario en C.
