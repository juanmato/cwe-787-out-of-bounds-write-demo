# CWE-787 · Out-of-bounds Write — Presentación + Demo en vivo

Material para la presentación de **Desarrollo Seguro de Software** (SANS Top 25 #5 — CWE-787,
*Out-of-bounds Write* / escritura fuera de límites).

## ⬇️ Descargar

Botón verde **Code → Download ZIP** (arriba a la derecha). Descomprimí y listo.

## ▶️ Correr la demo en Windows 11 (un clic, no instala nada)

1. Entrá a la carpeta `demo-cwe787/`.
2. Doble clic en **`INICIAR-DEMO.bat`** → se abre el navegador en http://localhost:8000.
   - Si aparece *"Windows protegió su PC"* (SmartScreen): **Más información → Ejecutar de todas formas**
     (el `.exe` no está firmado; el código fuente está acá al lado).
   - No pide Firewall: el servidor escucha solo en `localhost`.
3. Para detener: cerrá la ventana negra *"TechStore - Demo CWE-787"*.

En macOS/Linux: `cd demo-cwe787 && ./build.sh && ./bin/techstore`.

## 🛒 Qué muestra la demo

Un checkout de la tienda **TechStore**. El backend en C guarda el cupón en `coupon[16]`, y justo
al lado en memoria está `discount_percent`. La copia usa `strcpy()` **sin control de límite**:
un cupón de más de 16 bytes **se desborda y sobre-escribe el descuento** → el total queda en **$0**.

- Cupón inválido → $1.200 · Cupón `SALE25` → $900 · **Cupón malicioso (20×"A")** → **GRATIS**.
- Con el toggle **"Backend CORREGIDO"** (`strncpy` acotado), el mismo ataque queda bloqueado.

## 📦 Contenido

| Ruta | Qué es |
|------|--------|
| `CWE-787-Out-of-bounds-Write.pptx` | La presentación (10 slides). |
| `guion-y-preguntas.md` | Guion cronometrado + preguntas frecuentes con respuesta. |
| `demo-cwe787/` | La demo (Windows `.exe` autocontenido + fuentes en C + instructivo). |
| `demo-cwe787/README.md` | Instructivo detallado para reproducir la demo. |

> ⚠️ La demo **desactiva a propósito** las protecciones del compilador (stack canary, FORTIFY)
> para mostrar el CWE-787 "crudo". Son mitigaciones, no el arreglo: el fix real es acotar la copia.
