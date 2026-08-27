# CWE-787 · Out-of-bounds Write — Presentación + Demo en vivo

Material para la presentación de **Desarrollo Seguro de Software** (SANS Top 25 #5 — CWE-787,
*Out-of-bounds Write* / escritura fuera de límites).

## ▶️ Correr la demo (Docker)

Requiere Docker instalado y corriendo. Compila el servidor en C dentro del contenedor y lo sirve;
el código fuente queda a la vista, no se distribuye ningún binario.

```bash
cd demo-cwe787
docker build -t cwe787-demo .
docker run --rm -p 8000:8000 cwe787-demo
```

Abrí **http://localhost:8000**. Para detener: `Ctrl+C` en la terminal.

> ¿Sin Docker? Ver [`demo-cwe787/README.md`](demo-cwe787/README.md) para el build nativo con `gcc`/`clang`.

## 🛒 Qué muestra la demo

Un checkout de la tienda **TechStore**. El backend en C guarda el cupón en `coupon[16]`, y justo
al lado en memoria está `discount_percent`. La copia usa `strcpy()` **sin control de límite**:
un cupón de más de 16 bytes **se desborda y sobre-escribe el descuento** → el total queda en **$0**.

- Cupón inválido → $1.200 · Cupón `SALE25` → $900 · **Cupón malicioso (20×"A")** → **GRATIS**.
- Con el toggle **"Backend CORREGIDO"** (`strncpy` acotado), el mismo ataque queda bloqueado.

## 📦 Contenido

| Ruta | Qué es |
|------|--------|
| `CWE-787-Out-of-bounds-Write.pdf` | La presentación (10 slides). |
| `guion-y-preguntas.md` | Guion cronometrado + preguntas frecuentes con respuesta. |
| `demo-cwe787/` | La demo (Dockerfile + fuentes en C + instructivo). |
| `demo-cwe787/README.md` | Instructivo detallado para reproducir la demo. |

> ⚠️ La demo **desactiva a propósito** las protecciones del compilador (stack canary, FORTIFY)
> para mostrar el CWE-787 "crudo". Son mitigaciones, no el arreglo: el fix real es acotar la copia.
